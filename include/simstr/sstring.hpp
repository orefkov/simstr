
template<typename A, typename K>
concept storable_str = requires {
    A::is_str_storable == true;
    std::is_same_v<typename A::symb_type, K>;
};

template<typename A, typename K>
concept mutable_str = storable_str<A, K> && requires { A::is_str_mutable == true; };

template<typename A, typename K>
concept immutable_str = storable_str<A, K> && !mutable_str<A, K>;

template<typename A>
concept Allocatorable = requires(A& a, size_t size, void* void_ptr) {
    { a.allocate(size) } -> std::same_as<void*>;
    { a.deallocate(void_ptr) } noexcept -> std::same_as<void>;
};

/*
* База для объектов, владеющих строкой
* По прежнему ничего не знает о том, где наследник хранит строку и её размер
* Просто вызывает его методы для получения места, и заполняет его при необходимости.
* Работает только при создании объекта, не работает с модификацией строки после
* ее создания и гарантирует, что если вызываются эти методы, объект еще только
* создается, и какого-либо расшаривания данных еще не было.
* Эти методы должен реализовать класс-наследник, вызываются только при создании объекта
*   K* init(size_t size)     - выделить место для строки указанного размера, вернуть адрес
*   void create_empty()      - создать пустой объект
*   K* set_size(size_t size) - перевыделить место для строки, если при создании не угадали
*                              нужный размер и место нужно больше или меньше.
*                              Содержимое строки нужно оставить.
*
* K     - тип символов
* Impl  - тип наследника
*/
template<typename K, typename Impl, Allocatorable Allocator>
class str_storable {
public:
    using my_type = Impl;
    using traits = ch_traits<K>;
    using allocator_t = Allocator;

protected:
    [[_no_unique_address]]
    allocator_t allocator_;

    allocator_t& allocator() {
        return allocator_;
    }
    const allocator_t& allocator() const {
        return allocator_;
    }

    using uni = unicode_traits<K>;

    Impl& d() noexcept {
        return *static_cast<Impl*>(this);
    }
    const Impl& d() const noexcept {
        return *static_cast<const Impl*>(this);
    }
    template<typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    explicit constexpr str_storable(size_t size, Args&&... args) : allocator_(std::forward<Args>(args)...) {
        if (size)
            d().init(size);
        else
            d().create_empty();
    }

    template<StrType<K> From, typename Op1, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    static my_type changeCaseAscii(const From& f, const Op1& opMakeNeedCase, Args&&... args) {
        my_type result{std::forward<Args>(args)...};
        size_t len = f.length();
        if (len) {
            const K* source = f.symbols();
            K* destination = result.init(len);
            for (size_t l = 0; l < len; l++) {
                destination[l] = opMakeNeedCase(source[l]);
            }
        }
        return result;
    }

    template<typename T, bool Dummy = true>
    struct ChangeCase {
        template<typename From, typename Op1, typename... Args>
            requires std::is_constructible_v<allocator_t, Args...>
        static my_type changeCase(const From& f, const Op1& opChangeCase, Args&&... args) {
            my_type result{std::forward<Args>(args)...};
            ;
            size_t len = f.length();
            if (len) {
                opChangeCase(f.symbols(), len, result.init(len));
            }
            return result;
        }
    };
    // Для utf8 сделаем отдельную спецификацию, так как при смене регистра может изменится длина строки
    template<bool Dummy>
    struct ChangeCase<u8s, Dummy> {
        template<typename From, typename Op1, typename... Args>
            requires std::is_constructible_v<allocator_t, Args...>
        static my_type changeCase(const From& f, const Op1& opChangeCase, Args&&... args) {
            my_type result{std::forward<Args>(args)...};
            ;
            size_t len = f.length();
            if (len) {
                const K* ptr = f.symbols();
                K* pWrite = result.init(len);

                const u8s* source = ptr;
                u8s* dest = pWrite;
                size_t newLen = opChangeCase(source, len, dest, len);
                if (newLen < len) {
                    // Строка просто укоротилась
                    result.set_size(newLen);
                } else if (newLen > len) {
                    // Строка не влезла в буфер.
                    size_t readed = static_cast<size_t>(source - ptr);
                    size_t writed = static_cast<size_t>(dest - pWrite);
                    pWrite = result.set_size(newLen);
                    dest = pWrite + writed;
                    opChangeCase(source, len - readed, dest, newLen - writed);
                }
                pWrite[newLen] = 0;
            }
            return result;
        }
    };

public:
    using s_str = simple_str<K>;
    using s_str_nt = simple_str_nt<K>;

    inline static constexpr bool is_str_storable = true;

    template<typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    constexpr str_storable(Args&&... args) noexcept(std::is_nothrow_constructible_v<allocator_t, Args...>)
        : allocator_(std::forward<Args>(args)...) {
        d().create_empty();
    }

    // Конструктор из другого строкового объекта
    template<typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    constexpr str_storable(s_str other, Args&&... args) : allocator_(std::forward<Args>(args)...) {
        if (other.length()) {
            K* ptr = d().init(other.length());
            traits::copy(ptr, other.symbols(), other.length());
            ptr[other.length()] = 0;
        } else
            d().create_empty();
    }
    // Конструктор повторения
    template<typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    constexpr str_storable(size_t repeat, s_str pattern, Args&&... args) : allocator_(std::forward<Args>(args)...) {
        size_t l = pattern.length(), allLen = l * repeat;
        if (allLen) {
            K* ptr = d().init(allLen);
            for (size_t i = 0; i < repeat; i++) {
                traits::copy(ptr, pattern.symbols(), l);
                ptr += l;
            }
            *ptr = 0;
        } else
            d().create_empty();
    }

    template<typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    str_storable(size_t count, K pad, Args&&... args) : allocator_(std::forward<Args>(args)...) {
        if (count) {
            K* str = d().init(count);
            traits::assign(str, count, pad);
            str[count] = 0;
        } else
            d().create_empty();
    }

    // Конструктор из строкового выражения
    template<typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    constexpr str_storable(const StrExprForType<K> auto& expr, Args&&... args) : allocator_(std::forward<Args>(args)...) {
        size_t len = expr.length();
        if (len)
            *expr.place(d().init(len)) = 0;
        else
            d().create_empty();
    }

    // Конструктор из строкового источника с заменой
    template<StrType<K> From, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    str_storable(const From& f, s_str pattern, s_str repl, size_t offset = 0, size_t maxCount = 0, Args&&... args)
        : allocator_(std::forward<Args>(args)...) {

        auto findes = f.find_all(pattern, offset, maxCount);
        if (!findes.size()) {
            new (this) my_type{f};
            return;
        }
        size_t srcLen = f.length();
        size_t newSize = srcLen + static_cast<ptrdiff_t>(repl.len - pattern.len) * findes.size();

        if (!newSize) {
            new (this) my_type{};
            return;
        }

        K* ptr = d().init(newSize);
        const K* src = f.symbols();
        size_t from = 0;
        for (const auto& s: findes) {
            size_t copyLen = s - from;
            if (copyLen) {
                traits::copy(ptr, src + from, copyLen);
                ptr += copyLen;
            }
            if (repl.len) {
                traits::copy(ptr, repl.str, repl.len);
                ptr += repl.len;
            }
            from = s + pattern.len;
        }
        srcLen -= from;
        if (srcLen) {
            traits::copy(ptr, src + from, srcLen);
            ptr += srcLen;
        }
        *ptr = 0;
    }
    // for std::string compatibility
    const K* c_str() const {
        return d().symbols();
    }
    const K* data() const {
        return d().symbols();
    }

    s_str_nt to_nts(size_t from = 0) const {
        size_t len = d().length();
        if (from >= len) {
            from = len;
        }
        return {d().symbols() + from, len - from};
    }

    operator s_str_nt() const {
        return {d().symbols(), d().length()};
    }

    // Слияние контейнера строк
    template<typename T, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    static my_type join(const T& strings, s_str delimeter, bool tail = false, Args&&... args) {
        my_type result(std::forward<Args>(args)...);
        if (strings.size()) {
            if (strings.size() == 1 && (!delimeter.length() || !tail)) {
                result = strings.front();
            } else {
                size_t commonLen = 0;
                for (auto it = strings.begin(), e = strings.end(); it != e;) {
                    commonLen += it->length();
                    ++it;
                    if (it != e || tail)
                        commonLen += delimeter.length();
                }
                if (commonLen) {
                    K* ptr = result.init(commonLen);
                    for (auto it = strings.begin(), e = strings.end(); it != e;) {
                        size_t copyLen = it->length();
                        if (copyLen) {
                            traits::copy(ptr, it->symbols(), copyLen);
                            ptr += copyLen;
                        }
                        ++it;
                        if (delimeter.length() && (it != e || tail)) {
                            traits::copy(ptr, delimeter.symbols(), delimeter.length());
                            ptr += delimeter.length();
                        }
                    }
                    *ptr = 0;
                }
            }
        }
        return result;
    }
    // ascii версия upper
    template<StrType<K> From, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    static my_type uppered_only_ascii_from(const From& f, Args&&... args) {
        return changeCaseAscii(f, makeAsciiUpper<K>, std::forward<Args>(args)...);
    }

    // ascii версия lower
    template<StrType<K> From, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    static my_type lowered_only_ascii_from(const From& f, Args&&... args) {
        return changeCaseAscii(f, makeAsciiLower<K>, std::forward<Args>(args)...);
    }

    // Юникодная версия
    template<StrType<K> From, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    static my_type uppered_from(const From& f, Args&&... args) {
        return ChangeCase<K>::changeCase(f, uni::upper, std::forward<Args>(args)...);
    }

    // Юникодная версия
    template<StrType<K> From, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    static my_type lowered_from(const From& f, Args&&... args) {
        return ChangeCase<K>::changeCase(f, uni::lower, std::forward<Args>(args)...);
    }

    template<StrType<K> From, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    static my_type replaced_from(const From& f, s_str pattern, s_str repl, size_t offset = 0, size_t maxCount = 0, Args&&... args) {
        return my_type{f, pattern, repl, offset, maxCount, std::forward<Args>(args)...};
    }
};

template<typename K, Allocatorable Allocator>
class sstring;

struct printf_selector {
    template<typename K, typename... T>  requires (is_one_of_std_char_v<K>)
    static int snprintf(K* buffer, size_t count, const K* format, T&&... args) {
        if constexpr (std::is_same_v<K, u8s>) {
          #ifndef _WIN32
            return std::snprintf(buffer, count, format, std::forward<T>(args)...);
          #else
            // Поддерживает позиционные параметры
            return _sprintf_p(buffer, count, format, args...);
          #endif
        } else {
          #ifndef _WIN32
            return std::swprintf(to_one_of_std_char(buffer), count, to_one_of_std_char(format), args...);
          #else
            // Поддерживает позиционные параметры
            return _swprintf_p(to_one_of_std_char(buffer), count, to_one_of_std_char(format), args...);
          #endif
        }
    }
    template<typename K>  requires (is_one_of_std_char_v<K>)
    static int vsnprintf(K* buffer, size_t count, const K* format, va_list args) {
        if constexpr (std::is_same_v<K, u8s>) {
          #ifndef _WIN32
            return std::vsnprintf(buffer, count, format, args);
          #else
            // Поддерживает позиционные параметры
            return _vsprintf_p(buffer, count, format, args);
          #endif
        } else {
          #ifndef _WIN32
            return std::vswprintf(to_one_of_std_char(buffer), count, to_one_of_std_char(format), args);
          #else
            // Поддерживает позиционные параметры
            return _vswprintf_p(buffer, count, format, args);
          #endif
        }
    }
};

inline size_t grow2(size_t ret, size_t currentCapacity) {
    return ret <= currentCapacity ? ret : ret * 2;
}

/*
* Базовый класс работы с меняющимися inplace строками
* По прежнему ничего не знает о том, где наследник хранит строку и её размер
* Просто вызывает его методы для получения места, и заполняет его при необходимости.
* Для работы класс-наследник должен реализовать методы:
*   size_t length() const noexcept      - возвращает длину строки
*   const K* symbols() const            - возвращает указатель на начало строки
*   bool is_empty() const noexcept      - проверка, не пустая ли строка
*   K* str() noexcept                   - Неконстантный указатель на начало строки
*   K* set_size(size_t size)            - Изменить размер строки, как больше, так и меньше.
*                                         Содержимое строки нужно оставить.
*   K* reserve_no_preserve(size_t size) - выделить место под строку, старую можно не сохранять
*   size_t capacity() const noexcept    - вернуть текущую ёмкость строки, сколько может поместится
*                                         без аллокации
*
* K      - тип символов
* StrRef - тип хранилища куска строки
* Impl   - тип наследника
*/
template<typename K, typename StrRef, typename Impl>
class str_mutable {
public:
    using my_type = Impl;

private:
    Impl& d() {
        return *static_cast<Impl*>(this);
    }
    const Impl& d() const {
        return *static_cast<const Impl*>(this);
    }
    size_t _len() const noexcept {
        return d().length();
    }
    const K* _str() const noexcept {
        return d().symbols();
    }
    using simple_str = StrRef;
    using symb_type = K;
    using traits = ch_traits<K>;
    using uni = unicode_traits<K>;
    using uns_type = std::make_unsigned_t<K>;

    template<typename Op>
    Impl& make_trim_op(const Op& op) {
        simple_str me = static_cast<simple_str>(d()), pos = op(me);
        if (me.length() != pos.length()) {
            if (me.symbols() != pos.symbols())
                traits::move(const_cast<K*>(me.symbols()), pos.symbols(), pos.length());
            d().set_size(pos.length());
        }
        return d();
    }

    template<auto Op>
    Impl& commonChangeCase() {
        size_t len = _len();
        if (len)
            Op(_str(), len, str());
        return d();
    }
    // GCC до сих пор не позволяет делать внутри класса полную специализацию вложенного класса,
    // только частичную. Поэтому добавим неиспользуемый параметр шаблона.
    template<typename T, bool Dummy = true>
    struct CaseTraits {
        static Impl& upper(Impl& obj) {
            return obj.template commonChangeCase<unicode_traits<K>::upper>();
        }
        static Impl& lower(Impl& obj) {
            return obj.template commonChangeCase<unicode_traits<K>::lower>();
        }
    };

    template<auto Op>
    Impl& utf8CaseChange() {
        // Для utf-8 такая операция может изменить длину строки, поэтому для них делаем разные специализации
        size_t len = _len();
        if (len) {
            u8s* writePos = str();
            const u8s *startData = writePos, *readPos = writePos;
            size_t newLen = Op(readPos, len, writePos, len);
            if (newLen < len) {
                // Строка просто укоротилась
                d().set_size(newLen);
            } else if (newLen > len) {
                // Строка не влезла в буфер.
                size_t readed = static_cast<size_t>(readPos - startData);
                size_t writed = static_cast<size_t>(writePos - startData);
                d().set_size(newLen);
                startData = str(); // при изменении размера могло изменится
                readPos = startData + readed;
                writePos = const_cast<u8s*>(startData) + writed;
                Op(readPos, len - readed, writePos, newLen - writed);
            }
        }
        return d();
    }
    template<bool Dummy>
    struct CaseTraits<u8s, Dummy> {
        static Impl& upper(Impl& obj) {
            return obj.template utf8CaseChange<unicode_traits<u8s>::upper>();
        }
        static Impl& lower(Impl& obj) {
            return obj.template utf8CaseChange<unicode_traits<u8s>::lower>();
        }
    };

    template<TrimSides S, bool withSpaces, typename T, size_t N = const_lit_for<K, T>::Count>
    Impl& makeTrim(T&& pattern) {
        return make_trim_op(trim_operator<S, K, N - 1, withSpaces>{pattern});
    }

    template<TrimSides S, bool withSpaces>
    Impl& makeTrim(simple_str pattern) {
        return make_trim_op(trim_operator<S, K, 0, withSpaces>{{pattern}});
    }

public:
    K* str() noexcept {
        return d().str();
    }
    explicit operator K*() noexcept {
        return str();
    }
    // for std::string compatibility
    K* data() {
        return str();
    }

    Impl& trim() {
        return make_trim_op(SimpleTrim<TrimSides::TrimAll, K>{});
    }
    Impl& trim_left() {
        return make_trim_op(SimpleTrim<TrimSides::TrimLeft, K>{});
    }
    Impl& trim_right() {
        return make_trim_op(SimpleTrim<TrimSides::TrimRight, K>{});
    }

    template<typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    Impl& trim(T&& pattern) {
        return makeTrim<TrimSides::TrimAll, false>(pattern);
    }

    template<typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    Impl& trim_left(T&& pattern) {
        return makeTrim<TrimSides::TrimLeft, false>(pattern);
    }

    template<typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    Impl& trim_right(T&& pattern) {
        return makeTrim<TrimSides::TrimRight, false>(pattern);
    }

    template<typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    Impl& trim_with_spaces(T&& pattern) {
        return makeTrim<TrimSides::TrimAll, true>(pattern);
    }

    template<typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    Impl& trim_left_with_spaces(T&& pattern) {
        return makeTrim<TrimSides::TrimLeft, true>(pattern);
    }

    template<typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    Impl& trim_right_with_wpaces(T&& pattern) {
        return makeTrim<TrimSides::TrimRight, true>(pattern);
    }

    Impl& trim(simple_str pattern) {
        return pattern.length() ? makeTrim<TrimSides::TrimAll, false>(pattern) : d();
    }
    Impl& trim_left(simple_str pattern) {
        return pattern.length() ? makeTrim<TrimSides::TrimLeft, false>(pattern) : d();
    }
    Impl& trim_right(simple_str pattern) {
        return pattern.length() ? makeTrim<TrimSides::TrimRight, false>(pattern) : d();
    }
    Impl& trim_with_spaces(simple_str pattern) {
        return makeTrim<TrimSides::TrimAll, true>(pattern);
    }
    Impl& trim_left_with_spaces(simple_str pattern) {
        return makeTrim<TrimSides::TrimLeft, true>(pattern);
    }
    Impl& trim_right_with_spaces(simple_str pattern) {
        return makeTrim<TrimSides::TrimRight, true>(pattern);
    }

    Impl& upper_only_ascii() {
        K* ptr = str();
        for (size_t i = 0, l = _len(); i < l; i++, ptr++) {
            K s = *ptr;
            if (isAsciiLower(s))
                *ptr = s & ~0x20;
        }
        return d();
    }
    Impl& lower_only_ascii() {
        K* ptr = str();
        for (size_t i = 0, l = _len(); i < l; i++, ptr++) {
            K s = *ptr;
            if (isAsciiUpper(s))
                *ptr = s | 0x20;
        }
        return d();
    }

    Impl& upper() {
        // Для utf-8 такая операция может изменить длину строки, поэтому для них делаем разные специализации
        return CaseTraits<K>::upper(d());
    }
    Impl& lower() {
        // Для utf-8 такая операция может изменить длину строки, поэтому для них делаем разные специализации
        return CaseTraits<K>::lower(d());
    }

private:
    template<typename T>
    Impl& changeImpl(size_t from, size_t len, T expr) {
        size_t myLen = _len();
        if (from > myLen) {
            from = myLen;
        }
        if (from + len > myLen) {
            len = myLen - from;
        }
        K* buffer = str();
        size_t otherLen = expr.length();
        if (len == otherLen) {
            expr.place(buffer + from);
        } else {
            size_t tailLen = myLen - from - len;
            if (len > otherLen) {
                expr.place(buffer + from);
                traits::move(buffer + from + otherLen, buffer + from + len, tailLen);
                d().set_size(myLen - (len - otherLen));
            } else {
                buffer = d().set_size(myLen + otherLen - len);
                traits::move(buffer + from + otherLen, buffer + from + len, tailLen);
                expr.place(buffer + from);
            }
        }
        return d();
    }

    template<typename T>
    Impl& appendImpl(T expr) {
        if (size_t len = expr.length(); len) {
            size_t size = _len();
            expr.place(d().set_size(size + len) + size);
        }
        return d();
    }

    template<typename T>
    Impl& appendFromImpl(size_t pos, T expr) {
        if (pos > _len())
            pos = _len();
        if (size_t len = expr.length())
            expr.place(d().set_size(pos + len) + pos);
        else
            d().set_size(pos);
        return d();
    }

public:
    inline static constexpr bool is_str_mutable = true;

    Impl& append(simple_str other) {
        return appendImpl<simple_str>(other);
    }

    template<StrExprForType<K> A>
    Impl& append(const A& expr) {
        return appendImpl<const A&>(expr);
    }

    Impl& operator+=(simple_str other) {
        return appendImpl<simple_str>(other);
    }

    template<StrExprForType<K> A>
    Impl& operator+=(const A& expr) {
        return appendImpl<const A&>(expr);
    }

    Impl& append_in(size_t pos, simple_str other) {
        return appendFromImpl<simple_str>(pos, other);
    }

    template<StrExprForType<K> A>
    Impl& append_in(size_t pos, const A& expr) {
        return appendFromImpl<const A&>(pos, expr);
    }

    Impl& change(size_t from, size_t len, simple_str other) {
        return changeImpl<simple_str>(from, len, other);
    }

    template<StrExprForType<K> A>
    Impl& change(size_t from, size_t len, const A& expr) {
        return changeImpl<const A&>(from, len, expr);
    }

    Impl& insert(size_t to, simple_str other) {
        return changeImpl<simple_str>(to, 0, other);
    }
    template<StrExprForType<K> A>
    Impl& insert(size_t to, const A& expr) {
        return changeImpl<const A&>(to, 0, expr);
    }

    Impl& remove(size_t from, size_t to) {
        return changeImpl<const empty_expr<K>&>(from, to, {});
    }

    Impl& prepend(simple_str other) {
        return changeImpl<simple_str>(0, 0, other);
    }
    template<StrExprForType<K> A>
    Impl& prepend(const A& expr) {
        return changeImpl<const A&>(0, 0, expr);
    }

    Impl& replace(simple_str pattern, simple_str repl, size_t offset = 0, size_t maxCount = 0) {
        if (d().is_empty() || !pattern || offset + pattern.length() > _len())
            return d();
        if (!maxCount)
            maxCount--;
        if (pattern.length() == repl.length()) {
            // Заменяем inplace на подстроку такой же длины
            K* ptr = str();
            for (size_t i = 0; i < maxCount; i++) {
                offset = d().find(pattern, offset);
                if (offset == str::npos)
                    break;
                traits::copy(ptr + offset, repl.symbols(), repl.length());
                offset += repl.length();
            }
        } else if (pattern.length() > repl.length()) {
            // Заменяем на более короткий кусок, длина текста уменьшится, идём слева направо
            K* ptr = str();
            size_t posWrite = offset;
            for (size_t i = 0; i < maxCount; i++) {
                size_t idx = d().find(pattern, offset);
                if (idx == str::npos)
                    break;
                size_t lenOfPiece = idx - offset;
                if (posWrite < offset && lenOfPiece)
                    traits::move(ptr + posWrite, ptr + offset, lenOfPiece);
                posWrite += lenOfPiece;
                if (repl.length()) {
                    traits::copy(ptr + posWrite, repl.symbols(), repl.length());
                    posWrite += repl.length();
                }
                offset = idx + pattern.length();
            }
            size_t tailLen = _len() - offset;
            if (posWrite < offset && tailLen)
                traits::move(ptr + posWrite, ptr + offset, tailLen);
            d().set_size(posWrite + tailLen);
        } else {
            struct replace_grow_helper {
                replace_grow_helper(my_type& src, simple_str p, simple_str r, size_t mc, size_t d)
                    : source(src), pattern(p), repl(r), maxCount(mc), delta(d) {}
                my_type& source;
                simple_str pattern;
                simple_str repl;
                size_t maxCount;
                size_t delta;
                size_t all_delta{};
                K* reserve_for_copy{};
                size_t end_of_piece{};
                size_t total_length{};
                
                void replace(size_t offset) {
                    size_t finded[16] = {source.find(pattern, offset)};
                    if (finded[0] == str::npos) {
                        return;
                    }
                    maxCount--;
                    offset = finded[0] + pattern.length();
                    all_delta += delta;
                    size_t idx = 1;
                    for (size_t end = std::min(maxCount, std::size(finded)); idx < end; idx++, maxCount--) {
                        finded[idx] = source.find(pattern, offset);
                        if (finded[idx] == str::npos) {
                            break;
                        }
                        offset = finded[idx] + pattern.length();
                        all_delta += delta;
                    }
                    bool needMore = maxCount > 0 && idx == std::size(finded) && offset < source.length() - pattern.length();
                    if (needMore) {
                        replace(offset); // здесь произведутся замены в оставшемся хвосте
                    }
                    // Теперь делаем свои замены
                    if (!reserve_for_copy) {
                        // Только начинаем
                        end_of_piece = source.length();
                        total_length = end_of_piece + all_delta;
                        reserve_for_copy = source.alloc_for_copy(total_length);
                    }
                    K* dst_start = reserve_for_copy;
                    const K* src_start = source.symbols();
                    while(idx-- > 0) {
                        size_t pos = finded[idx] + pattern.length();
                        size_t lenOfPiece = end_of_piece - pos;
                        ch_traits<K>::move(dst_start + pos + all_delta, src_start + pos, lenOfPiece);
                        ch_traits<K>::copy(dst_start + pos + all_delta - repl.length(), repl.symbols(), repl.length());
                        all_delta -= delta;
                        end_of_piece = finded[idx];
                    }
                    if (!all_delta && reserve_for_copy != src_start) {
                        ch_traits<K>::copy(dst_start, src_start, finded[0]);
                    }
                }
            } helper(d(), pattern, repl, maxCount, repl.length() - pattern.length());
            helper.replace(offset);
            d().set_from_copy(helper.reserve_for_copy, helper.total_length);
        }
        return d();
    }

    template<StrType<K> From>
    Impl& replace_from(const From& f, simple_str pattern, simple_str repl, size_t offset = 0, size_t maxCount = 0) {
        if (pattern.length() >= repl.length()) {
            K* dst = d().reserve_no_preserve(f.length());
            const K* src = f.symbols();
            size_t delta = 0;
            if (maxCount == 0) {
                maxCount--;
            }
            size_t src_length = f.length(), start = 0;
            while (maxCount--) {
                offset = f.find(pattern, offset);
                if (offset == str::npos) {
                    break;
                }
                size_t piece_len = offset - start;
                if (piece_len) {
                    ch_traits<K>::copy(dst, src + start, piece_len);
                    dst += piece_len;
                }
                if (repl.length()) {
                    ch_traits<K>::copy(dst, repl.symbols(), repl.length());
                    dst += repl.length();
                }
                delta += pattern.length() - repl.length();
                offset += pattern.length();
                start = offset;
            }
            if (start < src_length) {
                ch_traits<K>::copy(dst, src + start, src_length - start);
            }
            d().set_size(src_length - delta);
        } else {
            d() = f;
            replace(pattern, repl, offset, maxCount);
        }
        return d();
    }

    // Реализация заполнения данными с проверкой на длину и перевыделением буфера в случае недостаточной длины.
    template<typename Op>
    my_type& fill(size_t from, const Op& fillFunction) {
        size_t size = _len();
        if (from > size)
            from = size;
        size_t capacity = d().capacity();
        K* ptr = str();
        capacity -= from;
        for (;;) {
            size_t needSize = (size_t)fillFunction(ptr + from, capacity);
            if (capacity >= needSize) {
                d().set_size(from + needSize);
                break;
            }
            ptr = from == 0 ? d().reserve_no_preserve(needSize) : d().set_size(from + needSize);
            capacity = d().capacity() - from;
        }
        return d();
    }
    // Реализация заполнения данными с проверкой на длину и перевыделением буфера в случае недостаточной длины.
    template<typename Op>
        requires std::is_invocable_v<Op, K*, size_t>
    my_type& operator<<(const Op& fillFunction) {
        return fill(0, fillFunction);
    }
    // Реализация добавления данных с проверкой на длину и перевыделением буфера в случае недостаточной длины.
    template<typename Op>
        requires std::is_invocable_v<Op, K*, size_t>
    my_type& operator<<=(const Op& fillFunction) {
        return fill(_len(), fillFunction);
    }
    template<typename Op>
        requires std::is_invocable_v<Op, my_type&>
    my_type& operator<<(const Op& fillFunction) {
        fillFunction(d());
        return d();
    }
    template<typename... T> requires (is_one_of_std_char_v<K>)
    my_type& printf_from(size_t from, const K* format, T&&... args) {
        size_t size = _len();
        if (from > size)
            from = size;
        size_t capacity = d().capacity();
        K* ptr = str();
        capacity -= from;

        int result = 0;
        // Тут грязный хак для u8s и wide_char. u8s версия snprintf сразу возвращает размер нужного буфера, если он мал
        // а swprintf - возвращает -1. Под windows оба варианта xxx_p - тоже возвращают -1.
        // Поэтому для них надо тупо увеличивать буфер наугад, пока не подойдет
        if constexpr (sizeof(K) == 1 && !isWindowsOs) {
            result = printf_selector::snprintf(ptr + from, capacity + 1, format, std::forward<T>(args)...);
            if (result > (int)capacity) {
                ptr = from == 0 ? d().reserve_no_preserve(result) : d().set_size(from + result);
                result = printf_selector::snprintf(ptr + from, result + 1, format, std::forward<T>(args)...);
            }
        } else {
            for (;;) {
                result = printf_selector::snprintf(ptr + from, capacity + 1, format, std::forward<T>(args)...);
                if (result < 0) {
                    // Не хватило буфера или ошибка конвертации.
                    // Попробуем увеличить буфер в два раза
                    capacity *= 2;
                    ptr = from == 0 ? d().reserve_no_preserve(capacity) : d().set_size(from + capacity);
                } else
                    break;
            }
        }
        if (result < 0)
            d().set_size(static_cast<size_t>(traits::length(_str())));
        else
            d().set_size(from + result);
        return d();
    }
    template<typename... T> requires (is_one_of_std_char_v<K>)
    my_type& printf(const K* pattern, T&&... args) {
        return printf_from(0, pattern, std::forward<T>(args)...);
    }
    template<typename... T> requires (is_one_of_std_char_v<K>)
    my_type& append_printf(const K* pattern, T&&... args) {
        return printf_from(_len(), pattern, std::forward<T>(args)...);
    }
    template<typename... T> requires (is_one_of_std_char_v<K>)
    my_type& format_from(size_t from, const FmtString<K, T...>& pattern, T&&... args) {
        size_t size = _len();
        if (from > size)
            from = size;
        size_t capacity = d().capacity();
        K* ptr = str();
        capacity -= from;

        auto result = std::format_to_n(to_one_of_std_char(ptr + from), capacity, pattern, std::forward<T>(args)...);
        if (result.size > (int)capacity) {
            ptr = from == 0 ? d().reserve_no_preserve((size_t)result.size) : d().set_size(from + (size_t)result.size);
            result = std::format_to_n(to_one_of_std_char(ptr + from), result.size, pattern, std::forward<T>(args)...);
        }
        d().set_size(from + (size_t)result.size);
        return d();
    }

    struct writer {
        my_type* store;
        K* ptr;
        const K* end;
        size_t max_write;
        size_t writed = 0;
        inline static K pad;
        K& operator*() const {
            return *ptr;
        }
        writer& operator++() {
            if (writed < max_write) {
                ++ptr;
                if (ptr == end) {
                    size_t l = ptr - store->begin();
                    store->set_size(l);
                    ptr = store->set_size(l + std::min(l / 2, size_t(8192))) + l;
                    end = store->end();
                }
            } else {
                ptr = &pad;
            }
            return *this;
        }
        writer operator++(int) {
            auto ret = *this;
            operator++();
            return ret;
        }

        writer(my_type& s, K* p, K* e, size_t ml) : store(&s), ptr(p), end(e), max_write(ml) {}
        writer() = default;
        writer(const writer&) = delete;
        writer& operator=(const writer&) noexcept = delete;
        writer(writer&&) noexcept = default;
        writer& operator=(writer&&) noexcept = default;
        using difference_type = int;
    };

    template<typename... T> requires (is_one_of_std_char_v<K>)
    my_type& vformat_from(size_t from, size_t max_write, simple_str pattern, T&&... args) {
        size_t size = _len();
        if (from > size)
            from = size;
        size_t capacity = d().capacity();
        K* ptr = str();

        if constexpr (std::is_same_v<K, u8s>) {
            auto result = std::vformat_to(
                writer{d(), ptr + from, ptr + capacity, max_write},
                std::basic_string_view<K>{pattern.symbols(), pattern.length()},
                std::make_format_args(args...));
            d().set_size(result.ptr - _str());
        } else {
            auto result = std::vformat_to(
                writer{d(), to_one_of_std_char(ptr + from), ptr + capacity, max_write},
                std::basic_string_view<wchar_t>{to_one_of_std_char(pattern.symbols()), pattern.length()},
                std::make_wformat_args(std::forward<T>(args)...));
            d().set_size(result.ptr - _str());
        }
        return d();
    }
    template<typename... T> requires (is_one_of_std_char_v<K>)
    my_type& format(const FmtString<K, T...>& pattern, T&&... args) {
        return format_from(0, pattern, std::forward<T>(args)...);
    }
    template<typename... T> requires (is_one_of_std_char_v<K>)
    my_type& append_formatted(const FmtString<K, T...>& pattern, T&&... args) {
        return format_from(_len(), pattern, std::forward<T>(args)...);
    }
    template<typename... T> requires (is_one_of_std_char_v<K>)
    my_type& vformat(simple_str pattern, T&&... args) {
        return vformat_from(0, -1, pattern, std::forward<T>(args)...);
    }

    template<typename... T> requires (is_one_of_std_char_v<K>)
    my_type& append_vformatted(simple_str pattern, T&&... args) {
        return vformat_from(_len(), -1, pattern, std::forward<T>(args)...);
    }

    template<typename... T> requires (is_one_of_std_char_v<K>)
    my_type& vformat_n(size_t max_write, simple_str pattern, T&&... args) {
        return vformat_from(0, max_write, pattern, std::forward<T>(args)...);
    }

    template<typename... T> requires (is_one_of_std_char_v<K>)
    my_type& append_vformatted_n(size_t max_write, simple_str pattern, T&&... args) {
        return vformat_from(_len(), max_write, pattern, std::forward<T>(args)...);
    }

    template<typename Op, typename... Args>
    my_type& with(const Op& fillFunction, Args&&... args) {
        fillFunction(d(), std::forward<Args>(args)...);
        return d();
    }
};

template<typename K>
struct SharedStringData {
    std::atomic_size_t ref_; // Счетчик ссылок

    SharedStringData() {
        ref_ = 1;
    }
    K* str() const {
        return (K*)(this + 1);
    }
    void incr() {
        ref_.fetch_add(1, std::memory_order_relaxed);
    }
    void decr(Allocatorable auto& allocator) {
        size_t val = ref_.fetch_sub(1, std::memory_order_relaxed);
        if (val == 1) {
            allocator.deallocate(this);
        }
    }
    static SharedStringData<K>* create(size_t l, Allocatorable auto& allocator) {
        size_t size = sizeof(SharedStringData<K>) + (l + 1) * sizeof(K);
        return new (allocator.allocate(size)) SharedStringData();
    }
    static SharedStringData<K>* from_str(const K* p) {
        return (SharedStringData<K>*)p - 1;
    }
    K* place(K* p, size_t len) {
        ch_traits<K>::copy(p, str(), len);
        return p + len;
    }
};

// Глобальный аллокатор для строк, один на весь процесс, может работать статически
class string_common_allocator {
public:
    void* allocate(size_t bytes) noexcept {
        return new char[bytes];
    }
    void deallocate(void* address) noexcept {
        delete [] static_cast<char*>(address);
    }
};

#ifndef default_str_allocator
#define default_str_allocator string_common_allocator
#endif

using allocator_string = default_str_allocator;

/*
* Локальная строка. Хранит в себе указатель на данные и длину строки, а за ней либо сами данные до N символов + нуль,
* либо если данные длиннее N, то размер выделенного буфера.
* При этом, если планируется потом результат переместить в sstring, то для динамического буфера
* выделяется +n байтов, чтобы потом не двигать данные.
* Так как у класса несколько базовых классов, MSVC не применяет автоматом empty base optimization,
* и без явного указания - вставит в начало класса пустые байты, сдвинув поле size на 4 байта.
* Укажем ему явно
*/
template<typename K, size_t N, bool forShared = false, Allocatorable Allocator = allocator_string>
class empty_bases lstring :
    public str_algs<K, simple_str<K>, lstring<K, N, forShared, Allocator>>,
    public str_mutable<K, simple_str<K>, lstring<K, N, forShared, Allocator>>,
    public str_storable<K, lstring<K, N, forShared, Allocator>, Allocator>,
    public from_utf_convertable<K, lstring<K, N, forShared, Allocator>> {
public:
    using symb_type = K;
    using my_type = lstring<K, N, forShared, Allocator>;
    using allocator_t = Allocator;

    enum : size_t { LocalCapacity = N | (sizeof(void*) / sizeof(K) - 1), AlocAlign = (alignof(std::max_align_t) / sizeof(K)) - 1};

protected:
    enum : size_t {
        extra = forShared ? sizeof(SharedStringData<K>) : 0,
    };

    using base_algs = str_algs<K, simple_str<K>, my_type>;
    using base_storable = str_storable<K, my_type, Allocator>;
    using base_mutable = str_mutable<K, simple_str<K>, my_type>;
    using base_utf = from_utf_convertable<K, my_type>;
    using traits = ch_traits<K>;

    friend base_storable;
    friend base_mutable;
    friend base_utf;
    friend class sstring<K, Allocator>;

    // Данные
    K* data_;
    size_t size_; // Поле не должно инициализироваться, так как может устанавливаться в базовых конструкторах

    union {
        size_t capacity_; // Поле не должно инициализироваться, так как может устанавливаться в базовых конструкторах
        K local_[LocalCapacity + 1];
    };

    void create_empty() {
        data_ = local_;
        size_ = 0;
        local_[0] = 0;
    }
    static size_t calc_capacity(size_t s) {
        size_t real_need = (s + 1) * sizeof(K) + extra;
        size_t aligned_alloced = (real_need + alignof(std::max_align_t) - 1) / alignof(std::max_align_t) * alignof(std::max_align_t);
        return (aligned_alloced - extra) / sizeof(K) - 1;
    }

    K* init(size_t s) {
        size_ = s;
        if (size_ > LocalCapacity) {
            s = calc_capacity(s);
            data_ = alloc_place(s);
            capacity_ = s;
        } else {
            data_ = local_;
        }
        return str();
    }
    // Методы для себя
    bool is_alloced() const noexcept {
        return data_ != local_;
    }

    void dealloc() {
        if (is_alloced()) {
            base_storable::allocator().deallocate(to_real_address(data_));
            data_ = local_;
        }
    }

    static K* to_real_address(void* ptr) {
        return reinterpret_cast<K*>(reinterpret_cast<u8s*>(ptr) - extra);
    }
    static K* from_real_address(void* ptr) {
        return reinterpret_cast<K*>(reinterpret_cast<u8s*>(ptr) + extra);
    }

    K* alloc_place(size_t newSize) {
        return from_real_address(base_storable::allocator().allocate((newSize + 1) * sizeof(K) + extra));
    }
    // Вызывается при replace, когда меняют на более длинную замену
    K* alloc_for_copy(size_t newSize) {
        if (capacity() >= newSize) {
            // Замена войдёт в текущий буфер
            return data_;
        }
        return alloc_place(calc_capacity(newSize));
    }
    // Вызывается после replace, когда меняли на более длинную замену, могли скопировать в новый буфер
    void set_from_copy(K* ptr, size_t newSize) {
        if (ptr != data_) {
            // Да, копировали в новый буфер
            dealloc();
            data_ = ptr;
            capacity_ = calc_capacity(newSize);
        }
        size_ = newSize;
        data_[newSize] = 0;
    }

public:
    using base_storable::base_storable;
    using base_utf::base_utf;

    lstring() = default;

    ~lstring() {
        dealloc();
    }

    // Копирование из другой строки
    lstring(const my_type& other) : base_storable(other.allocator()) {
        if (other.size_) {
            traits::copy(init(other.size_), other.symbols(), other.size_ + 1);
        }
    }
    // Копирование из другой строки но с другим аллокатором
    template<typename... Args>
        requires(sizeof...(Args) > 0 && std::is_convertible_v<allocator_t, Args...>)
    lstring(const my_type& other, Args&&... args) : base_storable(std::forward<Args>(args)...) {
        if (other.size_) {
            traits::copy(init(other.size_), other.symbols(), other.size_ + 1);
        }
    }

    // Конструктор из строкового литерала
    template<typename T, size_t I = const_lit_for<K, T>::Count, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    constexpr lstring(T&& value, Args&&... args) : base_storable(std::forward<Args>(args)...) {
        if constexpr (I > 1) {
            K* ptr = init(I - 1);
            traits::copy(ptr, value, I - 1);
            ptr[I - 1] = 0;
        } else
            create_empty();
    }

    // Перемещение из другой строки
    lstring(my_type&& other) noexcept : base_storable(std::move(other.allocator())) {
        if (other.size_) {
            size_ = other.size_;
            if (other.is_alloced()) {
                data_ = other.data_;
                capacity_ = other.capacity_;
            } else {
                data_ = local_;
                traits::copy(local_, other.local_, size_ + 1);
            }
            other.data_ = other.local_;
            other.size_ = 0;
            other.local_[0] = 0;
        }
    }

    template<typename Op, typename... Args>
        requires(std::is_constructible_v<Allocator, Args...> && (std::is_invocable_v<Op, my_type&> || std::is_invocable_v<Op, K*, size_t>))
    lstring(const Op& op, Args&&... args) : base_storable(std::forward<Args>(args)...) {
        this->operator<<(op);
    }

    // copy and swap для присваиваний здесь не очень применимо, так как для строк с большим локальным буфером лишняя копия даже перемещением будет дорого стоить
    // Поэтому реализуем копирующее и перемещающее присваивание отдельно
    my_type& operator=(const my_type& other) {
        // Так как между этими объектами не может быть косвенной зависимости, достаточно проверить только на равенство
        if (&other != this) {
            traits::copy(reserve_no_preserve(other.size_), other.data_, other.size_ + 1);
            size_ = other.size_;
        }
        return *this;
    }

    my_type& operator=(my_type&& other) noexcept {
        // Так как между этими объектами не может быть косвенной зависимости, достаточно проверить только на равенство
        if (&other != this) {
            if (other.is_alloced()) {
                dealloc();
                data_ = other.data_;
                capacity_ = other.capacity_;
            } else {
                traits::copy(data_, other.local_, other.size_ + 1);
            }
            base_storable::allocator() = std::move(other.allocator());
            size_ = other.size_;
            other.create_empty();
        }
        return *this;
    }

    my_type& assign(const K* other, size_t len) {
        if (len) {
            bool isIntersect = other >= data_ && other + len <= data_ + size_;
            if (isIntersect) {
                // Особый случай, нам пытаются присвоить кусок нашей же строки.
                // Просто переместим текст в буфере, и установим новый размер
                if (other > data_) {
                    traits::move(data_, other, len);
                }
            } else {
                traits::copy(reserve_no_preserve(len), other, len);
            }
        }
        size_ = len;
        data_[size_] = 0;
        return *this;
    }

    my_type& operator=(simple_str<K> other) {
        return assign(other.str, other.len);
    }

    template<typename T, size_t S = const_lit_for<K, T>::Count>
    my_type& operator=(T&& other) {
        return assign(other, S - 1);
    }

    // Если в строковом выражении что-либо ссылается на части этой же строки, то результат не определён
    my_type& operator=(const StrExprForType<K> auto& expr) {
        size_t newLen = expr.length();
        if (newLen) {
            expr.place(reserve_no_preserve(newLen));
        }
        size_ = newLen;
        data_[size_] = 0;
        return *this;
    }

    size_t length() const noexcept {
        return size_;
    }

    const K* symbols() const noexcept {
        return data_;
    }

    K* str() noexcept {
        return data_;
    }

    bool is_empty() const noexcept {
        return size_ == 0;
    }
    // for std::string compatibility
    bool empty() const noexcept {
        return size_ == 0;
    }

    size_t capacity() const noexcept {
        return is_alloced() ? capacity_ : LocalCapacity;
    }

    // Выделить буфер, достаточный для размещения newSize символов плюс завершающий ноль.
    // Содержимое буфера не определено, и не гарантируется сохранение старого содержимого.
    K* reserve_no_preserve(size_t newSize) {
        if (newSize > capacity()) {
            newSize = calc_capacity(newSize);
            K* newData = alloc_place(newSize);
            dealloc();
            data_ = newData;
            capacity_ = newSize;
        }
        return data_;
    }

    // Выделить буфер, достаточный для размещения newSize символов плюс завершающий ноль.
    // Содержимое строки сохраняется. При увеличении буфера размер выделяется не больше запрошенного.
    K* reserve(size_t newSize) {
        if (newSize > capacity()) {
            newSize = calc_capacity(newSize);
            K* newData = alloc_place(newSize);
            traits::copy(newData, data_, size_);
            dealloc();
            data_ = newData;
            capacity_ = newSize;
        }
        return data_;
    }

    // Устанавливает размер текущей строки. При необходимости перемещает данные в другой буфер
    // Содержимое сохраняется. При увеличении буфера размер выделяется не менее чем 2 старого размера буфера.
    K* set_size(size_t newSize) {
        size_t cap = capacity();
        if (newSize > cap) {
            size_t needPlace = newSize;
            if (needPlace < (cap + 1) * 2) {
                needPlace = (cap + 1) * 2 - 1;
            }
            reserve(needPlace);
        }
        size_ = newSize;
        data_[newSize] = 0;
        return data_;
    }

    size_t resize(size_t newSize, bool force = false) {
        size_t cap = capacity();
        if (newSize > capacity())
            set_size(newSize);
        if (force) {
            size_ = newSize;
            data_[newSize] = 0;
        }
        return length();
    }

    bool is_local() const noexcept {
        return !is_alloced();
    }

    void define_size() {
        size_t cap = capacity();
        for (size_t i = 0; i < cap; i++) {
            if (data_[i] == 0) {
                size_ = i;
                return;
            }
        }
        size_ = cap;
        data_[size_] = 0;
    }

    void shrink_to_fit() {
        size_t need_capacity = calc_capacity(size_);
        if (is_alloced() && capacity_ > need_capacity) {
            K* newData = size_ <= LocalCapacity ? local_ : alloc_place(need_capacity);
            traits::copy(newData, data_, size_ + 1);
            base_storable::allocator().deallocate(to_real_address(data_));
            data_ = newData;

            if (size_ > LocalCapacity) {
                capacity_ = need_capacity;
            }
        }
    }

    void clear() {
        set_size(0);
    }

    void reset() {
        dealloc();
        local_[0] = 0;
        size_ = 0;
    }
};

template<size_t N = 15>
using lstringa = lstring<u8s, N>;
template<size_t N = 15>
using lstringw = lstring<wchar_t, N>;
template<size_t N = 15>
using lstringu = lstring<u16s, N>;
template<size_t N = 15>
using lstringuu = lstring<u32s, N>;

template<size_t N = 15>
using lstringsa = lstring<u8s, N, true>;
template<size_t N = 15>
using lstringsw = lstring<wchar_t, N, true>;
template<size_t N = 15>
using lstringsu = lstring<u16s, N, true>;
template<size_t N = 15>
using lstringsuu = lstring<u32s, N, true>;


template<typename T, typename K = typename const_lit<T>::symb_type>
auto getLiteralType(T&&) {
    return K{};
};

template<size_t Arch, size_t L>
inline constexpr const size_t _local_count = 0;

template<>
inline constexpr const size_t _local_count<8, 1> = 23;
template<>
inline constexpr const size_t _local_count<8, 2> = 15;
template<>
inline constexpr const size_t _local_count<8, 4> = 7;
template<>
inline constexpr const size_t _local_count<4, 1> = 15;
template<>
inline constexpr const size_t _local_count<4, 2> = 11;
template<>
inline constexpr const size_t _local_count<4, 4> = 5;

template<typename T>
constexpr const size_t local_count = _local_count<sizeof(size_t), sizeof(T)>;

/*
* Класс с small string optimization плюс разделяемый иммутабельный буфер строки.
* Так как буфер строки в этом классе иммутабельный, то:
* Во-первых, нет нужды хранить размер выделенного буфера, мы его всё-равно не будем изменять
* Во-вторых, появляется ещё один тип строки - строка, инициализированная константным литералом.
* Для неё просто сохраняем указатель на символы, и не считаем ссылки.
* Таким образом, инициализация строкового объекта в программе литералом - ничего никуда не копирует -
* ни в себя, ни в динамическую память, и не стоит дороже по сравнению с инициализацией
* сырого указателя на строку, и даже ещё оптимальнее, так как ещё и сразу подставляет размер,
* а не вычисляет его в рантайме.
*
*     stringa text = "text or very very very long text"; // ничего не стоит!
*     stringa copy = anotherString; // Стоит только копирование байтов самого объекта плюс возможно один атомарный инкремент
*
* В случае разделяемого буфера размер строки всё-равно храним не в общем буфере, а в каждом объекте
* из-за SSO места всё-равно хватает, а в память лезть за длиной придётся меньше.
* Например, подсчитать сумму длин строк в векторе - пройдётся только по памяти в векторе.
*
* Размеры для x64:
* для u8s  - 24 байта, хранит строки до 23 символов + 0
* для u16s - 32 байта, хранит строки до 15 символов + 0
* для u32s - 32 байта, хранит строки до 7 символов + 0
*/

template<typename K, Allocatorable Allocator = allocator_string>
class empty_bases sstring :
    public str_algs<K, simple_str<K>, sstring<K, Allocator>>,
    public str_storable<K, sstring<K, Allocator>, Allocator>,
    public from_utf_convertable<K, sstring<K, Allocator>> {
public:
    using symb_type = K;
    using uns_type = std::make_unsigned_t<K>;
    using my_type = sstring<K, Allocator>;
    using allocator_t = Allocator;

    enum { LocalCount = local_count<K> };

protected:
    using base_algs = str_algs<K, simple_str<K>, my_type>;
    using base_storable = str_storable<K, my_type, Allocator>;
    using base_utf = from_utf_convertable<K, my_type>;
    using traits = ch_traits<K>;
    using uni = unicode_traits<K>;

    friend base_storable;
    friend base_utf;

    enum Types { Local, Constant, Shared };

    union {
        // Когда у нас короткая строка, она лежит в самом объекте, а в localRemain
        // пишется, сколько символов ещё можно вписать. Когда строка занимает всё
        // возможное место, то localRemain становится 0, type в этом случае тоже 0,
        // и в итоге после символов строки получается 0, как и надо!
        struct {
            K buf_[LocalCount]; // Локальный буфер строки
            uns_type localRemain_ : sizeof(uns_type) * 8 - 2;
            uns_type type_ : 2;
        };
        struct {
            union {
                const K* cstr_; // Указатель на конcтантную строку
                const K* sstr_; // Указатель на строку, перед которой лежит SharedStringData
            };
            size_t bigLen_; // Длина не локальной строки.
        };
    };

    void create_empty() {
        type_ = Local;
        localRemain_ = LocalCount;
        buf_[0] = 0;
    }
    K* init(size_t s) {
        if (s > LocalCount) {
            type_ = Shared;
            localRemain_ = 0;
            bigLen_ = s;
            sstr_ = SharedStringData<K>::create(s, base_storable::allocator())->str();
            return (K*)sstr_;
        } else {
            type_ = Local;
            localRemain_ = LocalCount - s;
            return buf_;
        }
    }

    K* set_size(size_t newSize) {
        // Вызывается при создании строки при необходимости изменить размер.
        // Других ссылок на shared buffer нет.
        size_t size = length();
        if (newSize != size) {
            if (type_ == Constant) {
                bigLen_ = newSize;
            } else {
                if (newSize <= LocalCount) {
                    if (type_ == Shared) {
                        SharedStringData<K>* str_buf = SharedStringData<K>::from_str(sstr_);
                        traits::copy(buf_, sstr_, newSize);
                        str_buf->decr(base_storable::allocator());
                    }
                    type_ = Local;
                    localRemain_ = LocalCount - newSize;
                } else {
                    if (type_ == Shared) {
                        if (newSize > size || (newSize > 64 && newSize <= size * 3 / 4)) {
                            K* newStr = SharedStringData<K>::create(newSize, base_storable::allocator())->str();
                            traits::copy(newStr, sstr_, newSize);
                            SharedStringData<K>::from_str(sstr_)->decr(base_storable::allocator());
                            sstr_ = newStr;
                        }
                    } else if (type_ == Local) {
                        K* newStr = SharedStringData<K>::create(newSize, base_storable::allocator())->str();
                        if (size)
                            traits::copy(newStr, buf_, size);
                        sstr_ = newStr;
                        type_ = Shared;
                        localRemain_ = 0;
                    }
                    bigLen_ = newSize;
                }
            }
        }
        K* str = type_ == Local ? buf_ : (K*)sstr_;
        str[newSize] = 0;
        return str;
    }

public:
    using base_storable::base_storable;
    using base_utf::base_utf;

    sstring() = default;

    template<typename... Args>
        requires(sizeof...(Args) > 0 && std::is_constructible_v<Allocator, Args...>)
    sstring(Args&&... args) : Allocator(std::forward<Args>(args)...) {}

    static const sstring<K> empty_str;

    ~sstring() {
        if (type_ == Shared) {
            SharedStringData<K>::from_str(sstr_)->decr(base_storable::allocator());
        }
    }

    sstring(const my_type& other) noexcept : base_storable(other.allocator()) {
        memcpy(buf_, other.buf_, sizeof(buf_) + sizeof(K));
        if (type_ == Shared)
            SharedStringData<K>::from_str(sstr_)->incr();
    }

    sstring(my_type&& other) noexcept : base_storable(std::move(other.allocator())) {
        memcpy(buf_, other.buf_, sizeof(buf_) + sizeof(K));
        other.create_empty();
    }

    // Конструктор перемещения из локальной строки
    template<size_t N>
    sstring(lstring<K, N, true, Allocator>&& src) : base_storable(std::move(src.allocator())) {
        size_t size = src.length();
        if (size) {
            if (src.is_alloced()) {
                // Там динамический буфер, выделенный с запасом для SharedStringData.
                K* str = src.str();
                if (size > LocalCount) {
                    // Просто присвоим его себе.
                    sstr_ = str;
                    bigLen_ = size;
                    type_ = Shared;
                    localRemain_ = 0;
                    new (SharedStringData<K>::from_str(str)) SharedStringData<K>();
                } else {
                    // Скопируем локально
                    type_ = Local;
                    localRemain_ = LocalCount - size;
                    traits::copy(buf_, str, size + 1);
                    // Освободим тот буфер, у локальной строки буфер не разделяется с другими
                    src.dealloc();
                }
            } else {
                // Копируем из локального буфера
                K* str = init(src.size_);
                traits::copy(str, src.symbols(), size + 1);
            }
            src.create_empty();
        } else
            create_empty();
    }

    // Инициализация из строкового литерала
    template<typename T, size_t N = const_lit_for<K, T>::Count, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    sstring(T&& s, Args&&... args) : base_storable(std::forward<Args>(args)...) {
        type_ = Constant;
        localRemain_ = 0;
        cstr_ = s;
        bigLen_ = N - 1;
    }

    void swap(my_type&& other) noexcept {
        char buf[sizeof(buf_) + sizeof(K)];
        memcpy(buf, buf_, sizeof(buf));
        memcpy(buf_, other.buf_, sizeof(buf));
        memcpy(other.buf_, buf, sizeof(buf));

        std::swap(base_storable::allocator(), other.allocator());
    }

    my_type& operator=(my_type other) noexcept {
        swap(std::move(other));
        return *this;
    }

    my_type& operator=(simple_str<K> other) {
        return operator=(my_type{other, base_storable::allocator()});
    }

    template<typename T, size_t N = const_lit_for<K, T>::Count>
    my_type& operator=(T&& other) {
        return operator=(my_type{other, base_storable::allocator()});
    }

    template<size_t N, bool forShared, typename A>
    my_type& operator=(const lstring<K, N, forShared, A>& other) {
        return operator=(my_type{other.to_str(), base_storable::allocator()});
    }

    template<size_t N>
    my_type& operator=(lstring<K, N, true, Allocator>&& other) {
        return operator=(my_type{std::move(other)});
    }

    my_type& operator=(const StrExprForType<K> auto& expr) {
        return operator=(my_type{expr, base_storable::allocator()});
    }

    my_type& make_empty() noexcept {
        if (type_ == Shared)
            SharedStringData<K>::from_str(sstr_)->decr(base_storable::allocator());
        create_empty();
        return *this;
    }

    const K* symbols() const noexcept {
        return type_ == Local ? buf_ : cstr_;
    }

    size_t length() const noexcept {
        return type_ == Local ? LocalCount - localRemain_ : bigLen_;
    }

    bool is_empty() const noexcept {
        return length() == 0;
    }
    // for std::string compatibility
    bool empty() const noexcept {
        return is_empty();
    }
    // Форматирование строки.
    template<typename... T>
    static my_type printf(const K* pattern, T&&... args) {
        return my_type{lstring<K, 256, true>{}.printf(pattern, std::forward<T>(args)...)};
    }

    template<typename... T>
    static my_type format(const FmtString<K, T...>& fmtString, T&&... args) {
        return my_type{lstring<K, 256, true, Allocator>{}.format(fmtString, std::forward<T>(args)...)};
    }
    template<typename... T>
    static my_type vformat(simple_str<K> fmtString, T&&... args) {
        return my_type{lstring<K, 256, true, Allocator>{}.vformat(fmtString, std::forward<T>(args)...)};
    }
};

template<typename K, Allocatorable Allocator>
inline const sstring<K> sstring<K, Allocator>::empty_str{};

template<size_t I>
struct digits_selector {
    using wider_type = uint16_t;
};

template<>
struct digits_selector<2> {
    using wider_type = uint32_t;
};

template<>
struct digits_selector<4> {
    using wider_type = uint64_t;
};

template<typename K, typename T>
constexpr size_t fromInt(K* bufEnd, T val) {
    const char* twoDigit =
        "0001020304050607080910111213141516171819"
        "2021222324252627282930313233343536373839"
        "4041424344454647484950515253545556575859"
        "6061626364656667686970717273747576777879"
        "8081828384858687888990919293949596979899";
    if (val) {
        need_sign<K, std::is_signed_v<T>, T> sign(val);
        K* itr = bufEnd;
        // Когда у нас минимальное отрицательное число, оно не меняется и остается меньше нуля
        if (std::is_signed_v<T> && val < 0) {
            // Возьмем две последние цифры
            const char* ptr = twoDigit - (val % 100) * 2;
            *--itr = static_cast<K>(ptr[1]);
            *--itr = static_cast<K>(ptr[0]);
            val /= 100;
            val = -val;
        }
        while (val >= 100) {
            const char* ptr = twoDigit + (val % 100) * 2;
            *--itr = static_cast<K>(ptr[1]);
            *--itr = static_cast<K>(ptr[0]);
            val /= 100;
        }
        if (val < 10) {
            *--itr = static_cast<K>('0' + val);
        } else {
            const char* ptr = twoDigit + val * 2;
            *--itr = static_cast<K>(ptr[1]);
            *--itr = static_cast<K>(ptr[0]);
        }
        sign.after(itr);
        return size_t(bufEnd - itr);
    }
    bufEnd[-1] = '0';
    return 1;
}

template<typename K, typename T>
struct expr_num {
    using symb_type = K;
    using my_type = expr_num<K, T>;

    enum { bufSize = 24 };
    mutable T value;
    mutable K buf[bufSize];

    expr_num(T t) : value(t) {}
    expr_num(expr_num<K, T>&& t) : value(t.value) {}

    size_t length() const noexcept {
        value = (T)fromInt(buf + bufSize, value);
        return (size_t)value;
    }
    K* place(K* ptr) const noexcept {
        ch_traits<K>::copy(ptr, buf + bufSize - (size_t)value, (size_t)value);
        return ptr + (size_t)value;
    }
};

template<StrExpr A, FromIntNumber T>
inline constexpr auto operator + (const A& a, T s) {
    return strexprjoin_c<A, expr_num<typename A::symb_type, T>>{a, s};
}

template<StrExpr A, FromIntNumber T>
inline constexpr auto operator + (T s, const A& a) {
    return strexprjoin_c<A, expr_num<typename A::symb_type, T>, false>{a, s};
}

template<typename K, typename T>
inline constexpr auto e_num(T t) {
    return expr_num<K, T>{t};
}

template<typename K>
simple_str_nt<K> select_str(simple_str_nt<u8s> s8, simple_str_nt<uws> sw, simple_str_nt<u16s> s16, simple_str_nt<u32s> s32) {
    if constexpr (std::is_same_v<K, u8s>)
        return s8;
    if constexpr (std::is_same_v<K, uws>)
        return sw;
    if constexpr (std::is_same_v<K, u16s>)
        return s16;
    if constexpr (std::is_same_v<K, u32s>)
        return s32;
}

#define uni_string(K, p) select_str<K>(p, L##p, u##p, U##p)

template<typename K> requires (is_one_of_std_char_v<K>)
struct expr_real {
    using symb_type = K;
    mutable K buf[40];
    mutable size_t l;
    double v;
    expr_real(double d) : v(d) {}
    expr_real(float d) : v(d) {}

    size_t length() const noexcept {
        printf_selector::snprintf(buf, 40, uni_string(K, "%.16g").str, v);
        l = (size_t)ch_traits<K>::length(buf);
        return l;
    }
    K* place(K* ptr) const noexcept {
        ch_traits<K>::copy(ptr, buf, l);
        return ptr + l;
    }
};

template<StrExpr A, typename R>
    requires(is_one_of_std_char_v<typename A::symb_type> && std::is_same_v<R, double> || std::is_same_v<R, float>)
inline constexpr auto operator+(const A& a, R s) {
    return strexprjoin_c<A, expr_real<typename A::symb_type>>{a, s};
}

template<StrExpr A, typename R>
    requires(is_one_of_std_char_v<typename A::symb_type> && std::is_same_v<R, double> || std::is_same_v<R, float>)
inline constexpr auto operator+(R s, const A& a) {
    return strexprjoin_c<A, expr_real<typename A::symb_type>, false>{a, s};
}

template<typename K> requires(is_one_of_std_char_v<K>)
inline constexpr auto e_real(double t) {
    return expr_real<K>{t};
}

/*
* Для создания строковых конкатенаций с векторами и списками, сджойненными константным разделителем
* K - тип символов строки
* T - тип контейнера строк (vector, list)
* I - длина разделителя в символах
* tail - добавлять разделитель после последнего элемента контейнера.
*        Если контейнер пустой, разделитель в любом случае не добавляется
*/

template<typename K, typename T, size_t I, bool tail, bool only_not_empty>
struct expr_join {
    using symb_type = K;
    using my_type = expr_join<K, T, I, tail, only_not_empty>;

    const T& s;
    const K* delim;

    constexpr size_t length() const noexcept {
        size_t l = 0;
        for (const auto& t: s) {
            size_t len = t.length();
            if (len > 0 || !only_not_empty) {
                if (I > 0 && l > 0) {
                    l += I;
                }
                l += len;
            }
        }
        return l + (tail && I > 0 && (l > 0 || (!only_not_empty && s.size() > 0))? I : 0);
    }
    constexpr K* place(K* ptr) const noexcept {
        if (s.empty()) {
            return ptr;
        }
        K* write = ptr;
        for (const auto& t: s) {
            size_t copyLen = t.length();
            if (I > 0 && write != ptr && (copyLen || !only_not_empty)) {
                ch_traits<K>::copy(write, delim, I);
                write += I;
            }
            if (copyLen) {
                ch_traits<K>::copy(write, t.symbols(), copyLen);
                write += copyLen;
            }
        }
        if (I > 0 && tail && (write != ptr || (!only_not_empty && s.size() > 0))) {
            ch_traits<K>::copy(write, delim, I);
            write += I;
        }
        return write;
    }
};

template<bool tail = false, bool only_not_empty = false, typename L, typename K = typename const_lit<L>::symb_type, size_t I = const_lit<L>::Count, typename T>
inline constexpr auto e_join(const T& s, L&& d) {
    return expr_join<K, T, I - 1, tail, only_not_empty>{s, d};
}

template<typename K, size_t N, size_t L>
struct expr_replaces {
    using symb_type = K;
    using my_type = expr_replaces<K, N, L>;
    simple_str<K> what;
    const K* pattern;
    const K* repl;
    mutable size_t first_, last_;

    constexpr expr_replaces(simple_str<K> w, const K* p, const K* r) : what(w), pattern(p), repl(r) {}

    constexpr size_t length() const {
        size_t l = what.length();
        if constexpr (N == L) {
            return l;
        }
        first_ = what.find(pattern, N, 0);
        if (first_ != str::npos) {
            last_ = first_ + N;
            for (;;) {
                l += L - N;
                size_t next = what.find(pattern, N, last_);
                if (next == str::npos) {
                    break;
                }
                last_ = next + N;
            }
        }
        return l;
    }
    constexpr K* place(K* ptr) const noexcept {
        if constexpr (N == L) {
            const K* from = what.symbols();
            for (size_t start = 0; start < what.length();) {
                size_t next = what.find(pattern, N, start);
                if (next == str::npos) {
                    next = what.length();
                }
                size_t delta = next - start;
                ch_traits<K>::copy(ptr,  from + start, delta);
                ptr += delta;
                ch_traits<K>::copy(ptr, repl, L);
                ptr += L;
                start = next + N;
            }
            return ptr;
        }
        if (first_ == str::npos) {
            return what.place(ptr);
        }
        const K* from = what.symbols();
        for (size_t start = 0, offset = first_; ;) {
            ch_traits<K>::copy(ptr, from + start, offset - start);
            ptr += offset - start;
            ch_traits<K>::copy(ptr, repl, L);
            ptr += L;
            start = offset + N;
            if (start >= last_) {
                size_t tail = what.length() - last_;
                ch_traits<K>::copy(ptr, from + last_, tail);
                ptr += tail;
                break;
            } else {
                offset = what.find(pattern, N, start);
            }
        }
        return ptr;
    }
};

template<typename K, typename T, size_t N = const_lit_for<K, T>::Count, typename X, size_t L = const_lit_for<K, X>::Count>
    requires(N > 1)
inline constexpr auto e_repl(simple_str<K> w, T&& p, X&& r) {
    return expr_replaces<K, N - 1, L - 1>{w, p, r};
}

template<typename K>
struct expr_replaced {
    using symb_type = K;
    using my_type = expr_replaced<K>;
    simple_str<K> what;
    const simple_str<K> pattern;
    const simple_str<K> repl;
    mutable size_t first_, last_;

    constexpr expr_replaced(simple_str<K> w, simple_str<K> p, simple_str<K> r) : what(w), pattern(p), repl(r) {}

    constexpr size_t length() const {
        size_t l = what.length();
        if (pattern.length() == repl.length()) {
            return l;
        }
        first_ = what.find(pattern);
        if (first_ != str::npos) {
            last_ = first_ + pattern.length();
            for (;;) {
                l += repl.length() - pattern.length();
                size_t next = what.find(pattern, last_);
                if (next == str::npos) {
                    break;
                }
                last_ = next + pattern.length();
            }
        }
        return l;
    }
    constexpr K* place(K* ptr) const noexcept {
        if (repl.length() == pattern.length()) {
            const K* from = what.symbols();
            for (size_t start = 0; start < what.length();) {
                size_t next = what.find(pattern, start);
                if (next == str::npos) {
                    next = what.length();
                }
                size_t delta = next - start;
                ch_traits<K>::copy(ptr,  from + start, delta);
                ptr += delta;
                ch_traits<K>::copy(ptr, repl.symbols(), repl.length());
                ptr += repl.length();
                start = next + pattern.length();
            }
            return ptr;
        }
        if (first_ == str::npos) {
            return what.place(ptr);
        }
        const K* from = what.symbols();
        for (size_t start = 0, offset = first_; ;) {
            ch_traits<K>::copy(ptr, from + start, offset - start);
            ptr += offset - start;
            ch_traits<K>::copy(ptr, repl.symbols(), repl.length());
            ptr += repl.length();
            start = offset + pattern.length();
            if (start >= last_) {
                size_t tail = what.length() - last_;
                ch_traits<K>::copy(ptr, from + last_, tail);
                ptr += tail;
                break;
            } else {
                offset = what.find(pattern, start);
            }
        }
        return ptr;
    }
};

template<bool UseVectorForReplace>
struct replace_search_result_store {
    size_t first_replace_ = str::npos;
    size_t first_idx_;
    size_t last_replace_;
};

template<>
struct replace_search_result_store<true> : std::vector<std::pair<size_t, size_t>> {};

// Строковое выражение для замены символов
template<typename K, bool UseVectorForReplace = true>
struct expr_replace_symbols {
    using symb_type = K;
    inline static const int BIT_SEARCH_TRESHHOLD = 4;

    const simple_str<K> source_;
    const std::vector<std::pair<K, simple_str<K>>>& replaces_;

    lstring<K, 32> pattern_;

    mutable replace_search_result_store<UseVectorForReplace> search_results_;

    uu8s bit_mask_[sizeof(K) == 1 ? 32 : 64]{};

    constexpr expr_replace_symbols(simple_str<K> source, const std::vector<std::pair<K, simple_str<K>>>& repl )
        : source_(source), replaces_(repl)
    {
        size_t pattern_len = replaces_.size();
        K* pattern = pattern_.set_size(pattern_len);

        for (size_t idx = 0; idx < replaces_.size(); idx++) {
            *pattern++ = replaces_[idx].first;
        }

        if (pattern_len >= BIT_SEARCH_TRESHHOLD) {
            for (size_t idx = 0; idx < pattern_len; idx++) {
                uu8s s = static_cast<uu8s>(pattern_[idx]);
                if constexpr (sizeof(K) == 1) {
                    bit_mask_[s >> 3] |= (1 << (s & 7));
                } else {
                    if (std::make_unsigned_t<K>(pattern_[idx]) > 255) {
                        bit_mask_[32 + (s >> 3)] |= (1 << (s & 7));
                    } else {
                        bit_mask_[s >> 3] |= (1 << (s & 7));
                    }
                }
            }
        }
    }

    size_t length() const {
        size_t l = source_.length();
        auto [fnd, num] = find_first_of(source_.str, source_.len);
        if (fnd == str::npos) {
            return l;
        }
        if constexpr (UseVectorForReplace) {
            search_results_.reserve((l >> 4) + 8);
            l = l - 1 + replaces_[num].second.len;
            search_results_.emplace_back(fnd, num);
            for (size_t start = fnd + 1;;) {
                auto [fnd, idx] = find_first_of(source_.str, source_.len, start);
                if (fnd == str::npos) {
                    break;
                }
                search_results_.emplace_back(fnd, idx);
                start = fnd + 1;
                l = l - 1 + replaces_[idx].second.len;
            }
        } else {
            l = l - 1 + replaces_[num].second.len;
            search_results_.first_replace_ = fnd;
            search_results_.first_idx_ = num;
            search_results_.last_replace_ = fnd + 1;
            for (;;) {
                auto [start, idx] = find_first_of(source_.str, source_.len, search_results_.last_replace_);
                if (start == str::npos) {
                    break;
                }
                search_results_.last_replace_ = start + 1;
                l = l - 1 + replaces_[idx].second.len;
            }
        }
        return l;
    }
    K* place(K* ptr) const noexcept {
        if constexpr (UseVectorForReplace) {
            size_t start = 0;
            const K* text = source_.str;
            for (const auto& [pos, num] : search_results_) {
                size_t delta = pos - start;
                ch_traits<K>::copy(ptr, text + start, delta);
                ptr += delta;
                ptr = replaces_[num].second.place(ptr);
                start = pos + 1;
            }
            if (size_t tail = source_.len - start; tail > 0) {
                ch_traits<K>::copy(ptr, text + start, tail);
                ptr += tail;
            }
        } else {
            if (search_results_.first_replace_ == -1) {
                return source_.place(ptr);
            }
            if (search_results_.first_replace_ > 0) {
                ch_traits<K>::copy(ptr, source_.str, search_results_.first_replace_);
                ptr += search_results_.first_replace_;
            }
            ptr = replaces_[search_results_.first_idx_].second.place(ptr);
            const K* text = source_.str;
            for (size_t start = search_results_.first_replace_ + 1; start < search_results_.last_replace_;) {
                auto [fnd, idx] = find_first_of(source_.str, source_.len, start);
                size_t delta = fnd - start;
                if (delta) {
                    ch_traits<K>::copy(ptr, text + start, delta);
                    ptr += delta;
                }
                ptr = replaces_[idx].second.place(ptr);
                start = fnd + 1;
            }
            if (size_t tail = source_.len - search_results_.last_replace_; tail > 0) {
                ch_traits<K>::copy(ptr, source_.str + search_results_.last_replace_, tail);
                ptr += tail;
            }
        }
        return ptr;
    }
    
protected:
    size_t index_of(K s) const {
        return pattern_.find(s);
    }

    bool is_in_mask(uu8s s) const {
        return (bit_mask_[s >> 3] & (1 << (s & 7))) != 0;
    }
    bool is_in_mask2(uu8s s) const {
        return (bit_mask_[32 + (s >> 3)] & (1 << (s & 7))) != 0;
    }

    bool is_in_pattern(K s, size_t& idx) const {
        if constexpr (sizeof(K) == 1) {
            if (is_in_mask(s)) {
                idx = index_of(s);
                return true;
            }
        } else {
            if (std::make_unsigned_t<const K>(s) > 255) {
                if (is_in_mask2(s)) {
                    return (idx = index_of(s)) != -1;
                }
            } else {
                if (is_in_mask(s)) {
                    idx = index_of(s);
                    return true;
                }
            }
        }
        return false;
    }

    std::pair<size_t, size_t> find_first_of(const K* text, size_t len,  size_t offset = 0) const {
        size_t pl = pattern_.length();
        if  (pl >= BIT_SEARCH_TRESHHOLD) {
            size_t idx;
            while (offset < len) {
                if (is_in_pattern(text[offset], idx)) {
                    return {offset, idx};
                }
                offset++;
            }
        } else {
            while (offset < len) {
                if (size_t idx = index_of(text[offset]); idx != -1) {
                    return {offset, idx};
                }
                offset++;
            }
        }
        return {-1, -1};
    }
};

// Строковое выражение для замены символов
template<typename K, size_t N, bool UseVectorForReplace>
struct expr_replace_const_symbols {
    using symb_type = K;
    inline static const int BIT_SEARCH_TRESHHOLD = 4;
    const K pattern_[N];
    const simple_str<K> source_;
    const simple_str<K> replaces_[N];

    mutable replace_search_result_store<UseVectorForReplace> search_results_;

    [[_no_unique_address]]
    uu8s bit_mask_[N >= BIT_SEARCH_TRESHHOLD ? (sizeof(K) == 1 ? 32 : 64) : 0]{};

    template<typename ... Repl> requires (sizeof...(Repl) == N * 2)
    constexpr expr_replace_const_symbols(simple_str<K> source, Repl&& ... repl) : expr_replace_const_symbols(0, source, std::forward<Repl>(repl)...) {}

    size_t length() const {
        size_t l = source_.length();
        auto [fnd, num] = find_first_of(source_.str, source_.len);
        if (fnd == str::npos) {
            return l;
        }
        if constexpr (UseVectorForReplace) {
            search_results_.reserve((l >> 4) + 8);
            l = l - 1 + replaces_[num].len;
            search_results_.emplace_back(fnd, num);
            for (size_t start = fnd + 1;;) {
                auto [fnd, idx] = find_first_of(source_.str, source_.len, start);
                if (fnd == str::npos) {
                    break;
                }
                search_results_.emplace_back(fnd, idx);
                start = fnd + 1;
                l = l - 1 + replaces_[idx].len;
            }
        } else {
            l = l - 1 + replaces_[num].len;
            search_results_.first_replace_ = fnd;
            search_results_.first_idx_ = num;
            search_results_.last_replace_ = fnd + 1;
            for (;;) {
                auto [start, idx] = find_first_of(source_.str, source_.len, search_results_.last_replace_);
                if (start == str::npos) {
                    break;
                }
                search_results_.last_replace_ = start + 1;
                l = l - 1 + replaces_[idx].len;
            }
        }
        return l;
    }
    K* place(K* ptr) const noexcept {
        if constexpr (UseVectorForReplace) {
            size_t start = 0;
            const K* text = source_.str;
            for (const auto& [pos, num] : search_results_) {
                size_t delta = pos - start;
                ch_traits<K>::copy(ptr, text + start, delta);
                ptr += delta;
                ptr = replaces_[num].place(ptr);
                start = pos + 1;
            }
            if (size_t tail = source_.len - start; tail > 0) {
                ch_traits<K>::copy(ptr, text + start, tail);
                ptr += tail;
            }
        } else {
            if (search_results_.first_replace_ == -1) {
                return source_.place(ptr);
            }
            if (search_results_.first_replace_ > 0) {
                ch_traits<K>::copy(ptr, source_.str, search_results_.first_replace_);
                ptr += search_results_.first_replace_;
            }
            ptr = replaces_[search_results_.first_idx_].place(ptr);
            const K* text = source_.str;
            for (size_t start = search_results_.first_replace_ + 1; start < search_results_.last_replace_;) {
                auto [fnd, idx] = find_first_of(source_.str, source_.len, start);
                size_t delta = fnd - start;
                if (delta) {
                    ch_traits<K>::copy(ptr, text + start, delta);
                    ptr += delta;
                }
                ptr = replaces_[idx].place(ptr);
                start = fnd + 1;
            }
            if (size_t tail = source_.len - search_results_.last_replace_; tail > 0) {
                ch_traits<K>::copy(ptr, source_.str + search_results_.last_replace_, tail);
                ptr += tail;
            }
        }
        return ptr;
    }
    
protected:
    template<typename ... Repl>
    constexpr expr_replace_const_symbols(int, simple_str<K> source, K s, simple_str<K> r, Repl&&... repl) :
        expr_replace_const_symbols(0, source, std::forward<Repl>(repl)..., std::make_pair(s, r)){}

    template<typename ... Repl> requires (sizeof...(Repl) == N)
    constexpr expr_replace_const_symbols(int, simple_str<K> source, Repl&&... repl) :
        source_(source), pattern_ {repl.first...}, replaces_{repl.second...}
    {
        if constexpr (N >= BIT_SEARCH_TRESHHOLD) {
            for (size_t idx = 0; idx < N; idx++) {
                uu8s s = static_cast<uu8s>(pattern_[idx]);
                if constexpr (sizeof(K) == 1) {
                    bit_mask_[s >> 3] |= 1 << (s & 7);
                } else {
                    if (std::make_unsigned_t<const K>(pattern_[idx]) > 255) {
                        bit_mask_[32 + (s >> 3)] |= 1 << (s & 7);
                    } else {
                        bit_mask_[s >> 3] |= 1 << (s & 7);
                    }
                }
            }
        }
    }

    template<size_t Idx>
    size_t index_of(K s) const {
        if constexpr (Idx < N) {
            return pattern_[Idx] == s ? Idx : index_of<Idx + 1>(s);
        }
        return -1;
    }
    bool is_in_mask(uu8s s) const {
        return (bit_mask_[s >> 3] & (1 <<(s & 7))) != 0;
    }
    bool is_in_mask2(uu8s s) const {
        return (bit_mask_[32 + (s >> 3)] & (1 <<(s & 7))) != 0;
    }

    bool is_in_pattern(K s, size_t& idx) const {
        if constexpr (N >= BIT_SEARCH_TRESHHOLD) {
            if constexpr (sizeof(K) == 1) {
                if (is_in_mask(s)) {
                    idx = index_of<0>(s);
                    return true;
                }
            } else {
                if (std::make_unsigned_t<const K>(s) > 255) {
                    if (is_in_mask2(s)) {
                        return (idx = index_of<0>(s)) != -1;
                    }
                } else {
                    if (is_in_mask(s)) {
                        idx = index_of<0>(s);
                        return true;
                    }
                }
            }
        }
        return false;
    }
    std::pair<size_t, size_t> find_first_of(const K* text, size_t len,  size_t offset = 0) const {
        if constexpr (N >= BIT_SEARCH_TRESHHOLD) {
            size_t idx;
            while (offset < len) {
                if (is_in_pattern(text[offset], idx)) {
                    return {offset, idx};
                }
                offset++;
            }
        } else {
            while (offset < len) {
                if (size_t idx = index_of<0>(text[offset]); idx != -1) {
                    return {offset, idx};
                }
                offset++;
            }
        }
        return {-1, -1};
    }
};

template<bool UseVector = true, typename K, typename ... Repl>
requires (sizeof...(Repl) % 2 == 0)
auto e_repl_const_symbols(simple_str<K> src, Repl&& ... other) {
    return expr_replace_const_symbols<K, sizeof...(Repl) / 2, UseVector>(src, std::forward<Repl>(other)...);
}

template<StrExpr A, StrExprForType<typename A::symb_type> B>
struct expr_choice {
    using symb_type = typename A::symb_type;
    using my_type = expr_choice<A, B>;
    const A& a;
    const B& b;
    bool choice;

    constexpr size_t length() const noexcept {
        return choice ? a.length() : b.length();
    }
    constexpr symb_type* place(symb_type* ptr) const noexcept {
        return choice ? a.place(ptr) : b.place(ptr);
    }
};

template<StrExpr A, StrExprForType<typename A::symb_type> B>
inline constexpr auto e_choice(bool c, const A& a, const B& b) {
    return expr_choice<A, B>{a, b, c};
}

template<typename K>
struct StoreType {
    constexpr StoreType(simple_str<K> s, size_t h) : str(s), hash(h) {}

    StoreType(const StoreType& other) : str(other.str), hash(other.hash) {
        if (other.stored) {
            stored = new K[str.length()];
            other.str.place(stored);
            str.str = stored;
        }
    }
    StoreType(StoreType&& other) : str(other.str), hash(other.hash), stored(other.stored) {
        other.stored = nullptr;
    }
    constexpr ~StoreType() {
        delete[] stored;
    }
    void save_str() {
        stored = new K[str.length()];
        str.place(stored);
        str.str = stored;
    }

    StoreType& operator=(StoreType) = delete;
    simple_str<K> str;
    size_t hash;
    K* stored{};

    // Функция валидна только при вызове через итератор мапы, т.е. когда значение сохранили.
    const simple_str_nt<K>& to_nt() const noexcept {
        return static_cast<const simple_str_nt<K>&>(str);
    }
    sstring<K> to_str() const noexcept {
        return str;
    }
};

using HashKeyA = StoreType<u8s>;
using HashKeyW = StoreType<u16s>;
using HashKeyU = StoreType<u32s>;

template<bool Wide>
struct fnv_const {
    static inline constexpr size_t basis = static_cast<size_t>(14695981039346656037ULL);
    static inline constexpr size_t prime = static_cast<size_t>(1099511628211ULL);
};

template<>
struct fnv_const<false> {
    static inline constexpr size_t basis = static_cast<size_t>(2166136261U);
    static inline constexpr size_t prime = static_cast<size_t>(16777619U);
};

using fnv = fnv_const<sizeof(size_t) == 8>;

inline constexpr size_t maxLenForHash = 32;

template<typename K>
inline constexpr size_t fnv_hash(const K* ptr, size_t l) {
    size_t h = fnv::basis;
    if (l <= maxLenForHash) {
        for (size_t i = 0; i < l; i++)
           h = (h ^ (std::make_unsigned_t<K>)ptr[i]) * fnv::prime;
    } else {
        unsigned delta = l / maxLenForHash, add = l % maxLenForHash, current = 0;
        for (size_t i = 0; i < maxLenForHash; i++) {
            h = (h ^ (std::make_unsigned_t<K>)ptr[current]) * fnv::prime;
            current += delta;
            if (add) {
                current++;
                add--;
            }
        }
    }
    return h;
};

template<typename K>
inline constexpr size_t fnv_hash_ia(const K* ptr, size_t l) {
    size_t h = fnv::basis;
    if (l <= maxLenForHash) {
        for (size_t i = 0; i < l; i++) {
            std::make_unsigned_t<K> s = (std::make_unsigned_t<K>)ptr[i];
            h = (h ^ (s >= 'A' && s <= 'Z' ? s | 0x20 : s)) * fnv::prime;
        }
    } else {
        unsigned delta = l / maxLenForHash, add = l % maxLenForHash, current = 0;
        for (size_t i = 0; i < maxLenForHash; i++) {
            std::make_unsigned_t<K> s = (std::make_unsigned_t<K>)ptr[current];
            h = (h ^ (s >= 'A' && s <= 'Z' ? s | 0x20 : s)) * fnv::prime;
            current += delta;
            if (add) {
                current++;
                add--;
            }
        }
    }
    return h;
};

template<typename T, typename K = typename const_lit<T>::symb_type, size_t N = const_lit<T>::Count>
inline constexpr size_t fnv_hash(T&& value) {
    size_t h = fnv::basis;
    if constexpr (N - 1 <= maxLenForHash) {
        for (size_t i = 0; i < N - 1; i++)
           h = (h ^ (std::make_unsigned_t<K>)value[i]) * fnv::prime;
    } else {
        unsigned delta = (N - 1) / maxLenForHash, add = (N - 1) % maxLenForHash, current = 0;
        for (size_t i = 0; i < maxLenForHash; i++) {
            h = (h ^ (std::make_unsigned_t<K>)value[current]) * fnv::prime;
            current += delta;
            if (add) {
                current++;
                add--;
            }
        }
    }
    return h;
};

template<typename T, typename K = typename const_lit<T>::symb_type, size_t N = const_lit<T>::Count>
inline constexpr size_t fnv_hash_ia(T&& value) {
    size_t h = fnv::basis;
    if constexpr (N - 1 <= maxLenForHash) {
        for (size_t i = 0; i < N - 1; i++) {
            std::make_unsigned_t<K> s = (std::make_unsigned_t<K>)value[i];
            h = (h ^ (s >= 'A' && s <= 'Z' ? s | 0x20 : s)) * fnv::prime;
        }
    } else {
        unsigned delta = (N - 1) / maxLenForHash, add = (N - 1) % maxLenForHash, current = 0;
        for (size_t i = 0; i < maxLenForHash; i++) {
            std::make_unsigned_t<K> s = (std::make_unsigned_t<K>)value[current];
            h = (h ^ (s >= 'A' && s <= 'Z' ? s | 0x20 : s)) * fnv::prime;
            current += delta;
            if (add) {
                current++;
                add--;
            }
        }
    }
    return h;
};

template<typename K>
inline consteval size_t fnv_hash_compile(const K* ptr, size_t l) {
    return fnv_hash(ptr, l);
};

template<typename K>
inline consteval size_t fnv_hash_ia_compile(const K* ptr, size_t l) {
    return fnv_hash_ia(ptr, l);
};

template<typename K>
struct streql;
template<typename K>
struct strhash;

/*
* Контейнер для более эффективного поиска по строковым ключам.
* Как unordered_map, но чуть лучше. В качестве ключей хранит simple_str вместе с посчитанным хешем.
* Чтобы simple_str было на что ссылаться, строковые значения ключей кладёт в список,
* с ключом запоминает позицию в списке. При удалении ключа удаляет и из списка.
* Позволяет использовать для поиска строковые литералы, не создавая для них объекта sstring.
* Начиная с С++20 в unordered_map появилась возможность для гетерогенного поиска по ключу с типом,
* отличным от типа хранящегося ключа. Однако удаление по прежнему только по типу ключа,
* что сводит на нет улучшения.
* Да и хэш тоже не хранит, каждый раз вычисляя заново.
*/
template<typename K, typename T, typename H = strhash<K>, typename E = streql<K>>
class hashStrMap : public std::unordered_map<StoreType<K>, T, H, E> {
protected:
    using InStore = StoreType<K>;

public:
    using my_type = hashStrMap<K, T, H, E>;
    using hash_t = std::unordered_map<InStore, T, H, E>;
    using hasher = H;

    hashStrMap() = default;
    hashStrMap(const my_type&) = default;
    hashStrMap(my_type&& o) = default;
    my_type& operator=(const my_type&) = default;
    my_type& operator=(my_type&&) = default;

    hashStrMap(std::initializer_list<std::pair<const StoreType<K>, T>>&& init) {
        for (const auto& e: init)
            emplace(e.first, e.second);
    }

    hashStrMap(std::initializer_list<std::pair<const sstring<K>, T>>&& init) {
        for (const auto& e: init)
            emplace(e.first, e.second);
    }

    // При входе хэш должен быть уже посчитан
    template<typename... ValArgs>
    auto try_emplace(const StoreType<K>& key, ValArgs&&... args) {
        auto it = hash_t::try_emplace(key, std::forward<ValArgs>(args)...);
        if (it.second && key.str.length()) {
            const_cast<InStore&>(it.first->first).save_str();
        }
        return it;
    }

    static StoreType<K> toStoreType(simple_str<K> key) {
        return {key, H{}(key)};
    }

    template<typename Key, typename... ValArgs>
        requires(std::is_convertible_v<Key, simple_str<K>>)
    auto try_emplace(Key&& key, ValArgs&&... args) {
        auto it = hash_t::try_emplace(toStoreType(key), std::forward<ValArgs>(args)...);
        if (it.second && it.first->first.str.length()) {
            const_cast<InStore&>(it.first->first).save_str();
        }
        return it;
    }

    template<typename... ValArgs>
    auto emplace(const StoreType<K>& key, ValArgs&&... args) {
        auto it = try_emplace(key, std::forward<ValArgs>(args)...);
        if (!it.second) {
            it.first->second = T{std::forward<ValArgs>(args)...};
        }
        return it;
    }

    template<typename Key, typename... ValArgs>
        requires(std::is_convertible_v<Key, simple_str<K>>)
    auto emplace(Key&& key, ValArgs&&... args) {
        auto it = try_emplace(std::forward<Key>(key), std::forward<ValArgs>(args)...);
        if (!it.second) {
            it.first->second = T{std::forward<ValArgs>(args)...};
        }
        return it;
    }

    template<typename Key>
        requires(std::is_convertible_v<Key, simple_str<K>>)
    auto& operator[](Key&& key) {
        return hash_t::operator[](toStoreType(key));
    }

    auto find(const StoreType<K>& key) const {
        return hash_t::find(key);
    }

    auto find(simple_str<K> key) const {
        return find(toStoreType(key));
    }

    auto find(const StoreType<K>& key) {
        return hash_t::find(key);
    }

    auto find(simple_str<K> key) {
        return find(toStoreType(key));
    }

    auto erase(typename hash_t::const_iterator it) {
        return hash_t::erase(it);
    }

    auto erase(const StoreType<K>& key) {
        auto it = hash_t::find(key);
        if (it != hash_t::end()) {
            hash_t::erase(it);
            return 1;
        }
        return 0;
    }

    auto erase(simple_str<K> key) {
        return erase(toStoreType(key));
    }

    bool lookup(const K* txt, T& val) const {
        auto it = find(e_s(txt));
        if (it != hash_t::end()) {
            val = it->second;
            return true;
        }
        return false;
    }

    bool lookup(simple_str<K> txt, T& val) const {
        auto it = find(txt);
        if (it != hash_t::end()) {
            val = it->second;
            return true;
        }
        return false;
    }

    void clear() {
        hash_t::clear();
    }
};

template<typename K>
struct streql {
    bool operator()(const StoreType<K>& _Left, const StoreType<K>& _Right) const {
        return _Left.hash == _Right.hash && _Left.str == _Right.str;
    }
};

template<typename K>
struct strhash { // hash functor for basic_string
    size_t operator()(simple_str<K> _Keyval) const {
        return fnv_hash(_Keyval.symbols(), _Keyval.length());
    }
    size_t operator()(const StoreType<K>& _Keyval) const {
        return _Keyval.hash;
    }
};

template<typename K>
struct streqlia {
    bool operator()(const StoreType<K>& _Left, const StoreType<K>& _Right) const {
        return _Left.hash == _Right.hash && _Left.str.equal_ia(_Right.str);
    }
};

template<typename K>
struct strhashia {
    size_t operator()(simple_str<K> _Keyval) const {
        return fnv_hash_ia(_Keyval.symbols(), _Keyval.length());
    }
    size_t operator()(const StoreType<K>& _Keyval) const {
        return _Keyval.hash;
    }
};

template<typename K>
struct streqliu {
    bool operator()(const StoreType<K>& _Left, const StoreType<K>& _Right) const {
        return _Left.hash == _Right.hash && _Left.str.equal_iu(_Right.str);
    }
};

template<typename K>
struct strhashiu {
    size_t operator()(simple_str<K> _Keyval) const {
        return unicode_traits<K>::hashiu(_Keyval.symbols(), _Keyval.length());
    }
    size_t operator()(const StoreType<K>& _Keyval) const {
        return _Keyval.hash;
    }
};

/*
 * Для построения длинных динамических строк конкатенацией мелких кусочков.
 * Выделяет по мере надобности отдельные блоки заданного размера (или кратного ему для больших вставок),
 * чтобы избежать релокации длинных строк.
 * После построения можно слить в одну строку.
 * Как показали замеры, работает медленнее, чем lstring +=, но экономнее по памяти.
 * Сам является строковым выражением.
*/
template<typename K>
class chunked_string_builder {
    using chunk_t = std::pair<std::unique_ptr<K[]>, size_t>;
    std::vector<chunk_t> chunks; // блоки и длина данных в них
    K* write{};                  // Текущая позиция записи
    size_t len{};                // Общая длина
    size_t remain{};             // Сколько осталось места в текущем блоке
    size_t align{1024};

public:
    using my_type = chunked_string_builder<K>;
    using symb_type = K;
    chunked_string_builder() = default;
    chunked_string_builder(size_t a) : align(a){};
    chunked_string_builder(const my_type&) = delete;
    chunked_string_builder(my_type&& other) noexcept
        : chunks(std::move(other.chunks)), write(other.write), len(other.len), remain(other.remain), align(other.align) {
        other.len = other.remain = 0;
        other.write = nullptr;
    }
    my_type& operator=(my_type other) noexcept {
        chunks.swap(other.chunks);
        write = other.write;
        len = other.len;
        remain = other.remain;
        align = other.align;
        other.len = other.remain = 0;
        other.write = nullptr;
        return *this;
    }

    // Добавление порции данных
    my_type& operator<<(simple_str<K> data) {
        if (data.len) {
            len += data.len;
            if (data.len <= remain) {
                // Добавляемые данные влезают в выделенный блок, просто скопируем их
                ch_traits<K>::copy(write, data.str, data.len);
                write += data.len;                // Сдвинем позицию  записи
                chunks.back().second += data.len; // Увеличим длину хранимых в блоке данных
                remain -= data.len;               // Уменьшим остаток места в блоке
            } else {
                // Не влезают
                if (remain) {
                    // Сначала запишем сколько влезет
                    ch_traits<K>::copy(write, data.str, remain);
                    data.len -= remain;
                    data.str += remain;
                    chunks.back().second += remain; // Увеличим длину хранимых в блоке данных
                }
                // Выделим новый блок и впишем в него данные
                size_t blockSize = (data.len + align - 1) / align * align; // Рассчитаем размер блока, кратного заданному выравниванию
                chunks.emplace_back(std::make_unique<K[]>(blockSize), data.len);
                write = chunks.back().first.get();
                ch_traits<K>::copy(write, data.str, data.len);
                write += data.len;
                remain = blockSize - data.len;
            }
        }
        return *this;
    }

    my_type& operator<<(const StrExprForType<K> auto& expr) {
        size_t l = expr.length();
        if (l) {
            if (l < remain) {
                write = expr.place(write);
                chunks.back().second += l;
                len += l;
                remain -= l;
            } else if (!remain) {
                size_t blockSize = (l + align - 1) / align * align; // Рассчитаем размер блока, кратного заданному выравниванию
                chunks.emplace_back(std::make_unique<K[]>(blockSize), l);
                write = expr.place(chunks.back().first.get());
                len += l;
                remain = blockSize - l;
            } else {
                auto store = std::make_unique<K[]>(l);
                expr.place(store.get());
                return operator<<({store.get(), l});
            }
        }
        return *this;
    }
    template<typename T>
    my_type& operator<<(T data)
        requires std::is_same_v<T, K>
    {
        return operator<<(expr_char<K>(data));
    }
    constexpr size_t length() const noexcept {
        return len;
    }

    void reset() {
        if (chunks.empty()) {
            return;
        }
        if (chunks.size() > 1) {
            remain = 0;
            chunks.resize(1);
        }
        remain += chunks[0].second;
        chunks[0].second = 0;
        len = 0;
        write = chunks[0].first.get();
    }

    constexpr K* place(K* p) const noexcept {
        for (const auto& block: chunks) {
            ch_traits<K>::copy(p, block.first.get(), block.second);
            p += block.second;
        }
        return p;
    }

    template<typename Op>
    void out(const Op& o) const {
        for (const auto& block: chunks)
            o(block.first.get(), block.second);
    }

    bool is_continuous() const {
        if (chunks.size()) {
            const char* ptr = chunks.front().first.get();
            for (const auto& chunk: chunks) {
                if (chunk.first.get() != ptr)
                    return false;
                ptr += chunk.second;
            }
        }
        return true;
    }
    const K* begin() const {
        return chunks.size() ? chunks.front().first.get() : simple_str_nt<K>::empty_str.str;
    }

    void clear() {
        chunks.clear();
        write = nullptr;
        len = 0;
        remain = 0;
    }
    struct portion_store {
        typename decltype(chunks)::const_iterator it, end;
        size_t writedFromCurrentChunk;

        bool is_end() {
            return it == end;
        }
        size_t store(K* buffer, size_t size) {
            size_t writed = 0;
            while (size && !is_end()) {
                size_t remain = it->second - writedFromCurrentChunk;
                size_t write = std::min(size, remain);
                ch_traits<K>::copy(buffer, it->first.get() + writedFromCurrentChunk, write);
                writed += write;
                remain -= write;
                size -= write;
                if (!remain) {
                    ++it;
                    writedFromCurrentChunk = 0;
                } else
                    writedFromCurrentChunk += write;
            }
            return writed;
        }
    };
    portion_store get_portion() const {
        return {chunks.begin(), chunks.end(), 0};
    }
    const auto& data() const {
        return chunks;
    }
};

using stringa = sstring<u8s>;
using stringw = sstring<wchar_t>;
using stringu = sstring<u16s>;
using stringuu = sstring<u32s>;

template<typename T>
using hashStrMapA = hashStrMap<u8s, T, strhash<u8s>, streql<u8s>>;
template<typename T>
using hashStrMapAIA = hashStrMap<u8s, T, strhashia<u8s>, streqlia<u8s>>;
template<typename T>
using hashStrMapAIU = hashStrMap<u8s, T, strhashiu<u8s>, streqliu<u8s>>;

template<typename T>
using hashStrMapW = hashStrMap<wchar_t, T, strhash<wchar_t>, streql<wchar_t>>;
template<typename T>
using hashStrMapWIA = hashStrMap<wchar_t, T, strhashia<wchar_t>, streqlia<wchar_t>>;
template<typename T>
using hashStrMapWIU = hashStrMap<wchar_t, T, strhashiu<wchar_t>, streqliu<wchar_t>>;

template<typename T>
using hashStrMapU = hashStrMap<u16s, T, strhash<u16s>, streql<u16s>>;
template<typename T>
using hashStrMapUIA = hashStrMap<u16s, T, strhashia<u16s>, streqlia<u16s>>;
template<typename T>
using hashStrMapUIU = hashStrMap<u16s, T, strhashiu<u16s>, streqliu<u16s>>;

template<typename T>
using hashStrMapUU = hashStrMap<u32s, T, strhash<u32s>, streql<u32s>>;
template<typename T>
using hashStrMapUUIA = hashStrMap<u32s, T, strhashia<u32s>, streqlia<u32s>>;
template<typename T>
using hashStrMapUUIU = hashStrMap<u32s, T, strhashiu<u32s>, streqliu<u32s>>;

inline constexpr simple_str_nt<u8s> utf8_bom{"\xEF\xBB\xBF", 3}; // NOLINT

inline simple_str_nt<u8s> operator""_ss(const u8s* ptr, size_t l) {
    return simple_str_nt<u8s>{ptr, (size_t)l};
}

inline simple_str_nt<uws> operator""_ss(const uws* ptr, size_t l) {
    return simple_str_nt<uws>{ptr, (size_t)l};
}

inline simple_str_nt<u16s> operator""_ss(const u16s* ptr, size_t l) {
    return simple_str_nt<u16s>{ptr, (size_t)l};
}

inline simple_str_nt<u32s> operator""_ss(const u32s* ptr, size_t l) {
    return simple_str_nt<u32s>{ptr, (size_t)l};
}

consteval StoreType<u8s> operator""_h(const u8s* ptr, size_t l) {
    return StoreType<u8s>{{ptr, (size_t)l}, fnv_hash_compile(ptr, (size_t)l)};
}

consteval StoreType<u8s> operator""_ia(const u8s* ptr, size_t l) {
    return StoreType<u8s>{{ptr, (size_t)l}, fnv_hash_ia_compile(ptr, (size_t)l)};
}

inline StoreType<u8s> operator""_iu(const u8s* ptr, size_t l) {
    return StoreType<u8s>{{ptr, (size_t)l}, strhashiu<u8s>{}(simple_str<u8s>{ptr, (size_t)l})};
}

consteval StoreType<u16s> operator""_h(const u16s* ptr, size_t l) {
    return StoreType<u16s>{{ptr, (size_t)l}, fnv_hash_compile(ptr, (size_t)l)};
}

consteval StoreType<u16s> operator""_ia(const u16s* ptr, size_t l) {
    return StoreType<u16s>{{ptr, (size_t)l}, fnv_hash_ia_compile(ptr, (size_t)l)};
}

inline StoreType<u16s> operator""_iu(const u16s* ptr, size_t l) {
    return StoreType<u16s>{{ptr, (size_t)l}, strhashiu<u16s>{}(simple_str<u16s>{ptr, (size_t)l})};
}

consteval StoreType<u32s> operator""_h(const u32s* ptr, size_t l) {
    return StoreType<u32s>{{ptr, (size_t)l}, fnv_hash_compile(ptr, (size_t)l)};
}

consteval StoreType<u32s> operator""_ia(const u32s* ptr, size_t l) {
    return StoreType<u32s>{{ptr, (size_t)l}, fnv_hash_ia_compile(ptr, (size_t)l)};
}

inline StoreType<u32s> operator""_iu(const u32s* ptr, size_t l) {
    return StoreType<u32s>{{ptr, (size_t)l}, strhashiu<u32s>{}(simple_str<u32s>{ptr, (size_t)l})};
}

} // namespace simstr

template<typename K>
struct std::formatter<simstr::simple_str<K>, K> : std::formatter<std::basic_string_view<K>, K> {
    // Define format() by calling the base class implementation with the wrapped value
    template<typename FormatContext>
    auto format(simstr::simple_str<K> t, FormatContext& fc) const {
        return std::formatter<std::basic_string_view<K>, K>::format({t.str, t.len}, fc);
    }
};

template<typename K>
struct std::formatter<simstr::simple_str_nt<K>, K> : std::formatter<std::basic_string_view<K>, K> {
    // Define format() by calling the base class implementation with the wrapped value
    template<typename FormatContext>
    auto format(simstr::simple_str_nt<K> t, FormatContext& fc) const {
        return std::formatter<std::basic_string_view<K>, K>::format({t.str, t.len}, fc);
    }
};

template<typename K>
struct std::formatter<simstr::sstring<K>> : std::formatter<std::basic_string_view<K>, K> {
    // Define format() by calling the base class implementation with the wrapped value
    template<typename FormatContext>
    auto format(const simstr::sstring<K>& t, FormatContext& fc) const {
        return std::formatter<std::basic_string_view<K>, K>::format({t.symbols(), t.length()}, fc);
    }
};

template<typename K, unsigned N, bool S>
struct std::formatter<simstr::lstring<K, N, S>> : std::formatter<std::basic_string_view<K>, K> {
    // Define format() by calling the base class implementation with the wrapped value
    template<typename FormatContext>
    auto format(const simstr::lstring<K, N, S>& t, FormatContext& fc) const {
        return std::formatter<std::basic_string_view<K>, K>::format({t.symbols(), t.length()}, fc);
    }
};

inline std::ostream& operator<<(std::ostream& stream, simstr::ssa text) {
    return stream << std::string_view{text.symbols(), text.length()};
}

inline std::wostream& operator<<(std::wostream& stream, simstr::ssw text) {
    return stream << std::wstring_view{text.symbols(), text.length()};
}

inline std::ostream& operator<<(std::ostream& stream, const simstr::stringa& text) {
    return stream << std::string_view{text.symbols(), text.length()};
}

template<size_t N, bool S, simstr::Allocatorable A>
inline std::ostream& operator<<(std::ostream& stream, const simstr::lstring<simstr::u8s, N, S, A>& text) {
    return stream << std::string_view{text.symbols(), text.length()};
}

inline std::wostream& operator<<(std::wostream& stream, const simstr::stringw& text) {
    return stream << std::wstring_view{text.symbols(), text.length()};
}

template<size_t N, bool S, simstr::Allocatorable A>
inline std::wostream& operator<<(std::wostream& stream, const simstr::lstring<simstr::uws, N, S, A>& text) {
    return stream << std::wstring_view{text.symbols(), text.length()};
}
