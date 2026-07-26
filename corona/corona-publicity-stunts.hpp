#pragma once



namespace corona
{

    class CoronaDropSource : public IDropSource {
    public:
        // Implement IUnknown methods
        HRESULT __stdcall QueryInterface(REFIID riid, void** ppv) override {
            if (riid == IID_IUnknown || riid == IID_IDropSource) {
                *ppv = static_cast<IDropSource*>(this);
                AddRef();
                return S_OK;
            }
            *ppv = nullptr;
            return E_NOINTERFACE;
        }

        ULONG __stdcall AddRef() override {
            return InterlockedIncrement(&m_refCount);
        }

        ULONG __stdcall Release() override {
            ULONG count = InterlockedDecrement(&m_refCount);
            if (count == 0) {
                delete this;
            }
            return count;
        }

        // Implement IDropSource methods
        HRESULT __stdcall QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState) override {
            if (fEscapePressed) return DRAGDROP_S_CANCEL;
            if (!(grfKeyState & MK_LBUTTON)) return DRAGDROP_S_DROP;
            return S_OK;
        }

        HRESULT __stdcall GiveFeedback(DWORD dwEffect) override {
            return DRAGDROP_S_USEDEFAULTCURSORS;
        }

    private:
        LONG m_refCount = 1;
    };

    // Function to start the drag-and-drop operation
    void StartDragDropOperation() {
        CoronaDropSource* pDropSource = new CoronaDropSource();
        IDataObject* pDataObject = nullptr; // CreateMyDataObject(); // Implement this function to create your data object
        DWORD dwEffect;
        DoDragDrop(pDataObject, pDropSource, DROPEFFECT_COPY | DROPEFFECT_MOVE, &dwEffect);
        pDropSource->Release();
        pDataObject->Release();
    }

    namespace factoring
    {

        class symbol
        {
        public:

            symbol() = default;
            symbol(const symbol& _src) = default;
            symbol(symbol&& _src) = default;
            symbol& operator=(const symbol& _src) = default;
            symbol& operator=(symbol&& _src) = default;

            int digit;
            int version;

            bool operator < (const symbol& _other) const
            {
                return std::tie(digit, version) < std::tie(_other.digit, _other.version);
            }
        };

        class symbol_pair
        {
        public:
            symbol  x, 
                    y, // operands
                    r; // result

            symbol_pair() = default;
            symbol_pair(const symbol_pair& _src) = default;
            symbol_pair(symbol_pair&& _src) = default;
            symbol_pair& operator=(const symbol_pair& _src) = default;
            symbol_pair& operator=(symbol_pair&& _src) = default;

            symbol_pair(symbol _x, symbol _y, symbol _r) : x(_x), y(_y), r(_r)
            {
            }

            symbol_pair(symbol _x, symbol _y) : x(_x), y(_y)
            {
            }

            bool operator < (const symbol_pair& _other) const
            {
                return std::tie(x, y) < std::tie(_other.x, _other.y);
            }
        };

        class function
        {
        public:

            function() = default;
            function(const function& _src) = default;
            function(function&& _src) = default;
            function& operator=(const function& _src) = default;
            function& operator=(function&& _src) = default;

            std::map<int, int> symbol_version;
            std::set<symbol_pair> symbol_pairs;

            symbol get_symbol(int _digit)
            {
                symbol s;
                s.digit = _digit;

                auto it = symbol_version.find(_digit);
                if (it != symbol_version.end()) {
                    s.version = it->second;
                    it->second++;
                }
                else {
                    symbol_version.insert({ _digit, 0 });
                }
                return s;
            }

            symbol calculate(symbol& _x, symbol& _y)
            {
                symbol result;
                auto it = symbol_pairs.find(symbol_pair{ _x, _y, {} });
                if (it != symbol_pairs.end()) {
                    result = it->r;
                }
                return result;
            }

            static function create_addition_function(int _max_digit)
            {
                function f;
                for (int x = 0; x <= _max_digit; x++) {
                    symbol xv = f.get_symbol(x);
                    for (int y = 0; y <= _max_digit; y++) {
                        symbol yv = f.get_symbol(y);
                        symbol rv = f.get_symbol(x + y);
                        symbol_pair spi(xv, yv, rv);
                        f.symbol_pairs.insert(spi);
                    }
                }
                return f;
            }

            static function create_modulos_function(int _max_digit)
            {
                function f;
                symbol yv = f.get_symbol(_max_digit + 1);
                for (int x = 0; x <= _max_digit; x++) {
                    symbol xv = f.get_symbol(x);
                    symbol rv = f.get_symbol(x % (_max_digit + 1));
                    symbol_pair spi(xv, yv, rv);
                    f.symbol_pairs.insert(spi);
                }
                return f;
            }

            static function create_multiplication_function(int _max_digit)
            {
                function f;
                for (int x = 0; x <= _max_digit; x++) {
                    symbol xv = f.get_symbol(x);
                    for (int y = 0; y <= _max_digit; y++) {
                        symbol yv = f.get_symbol(y);
                        symbol rv = f.get_symbol(x * y);
                        symbol_pair spi(xv, yv, rv);
                        f.symbol_pairs.insert(spi);
                    }
                }
                return f;
            }
        };

        class number 
        {
        public:
            std::vector<symbol> symbols;

            number() = default;
            number(const number& _src) = default;
            number(number&& _src) = default;
            number& operator=(const number& _src) = default;
            number& operator=(number&& _src) = default;
            number(std::string _number)
            {
                for (char c : _number) {
                    if (std::isdigit(c)) {
                        int digit = c - '0';
                        symbol t;
                        t.digit = digit;
                        t.version = 0;
                        symbols.push_back(t);
                    }
                }
                std::reverse(symbols.begin(), symbols.end());
            }

            bool empty() const
            {
                return symbols.empty();
            }

            std::string to_string() const
            {
                std::string result;
                for (auto it = symbols.rbegin(); it != symbols.rend(); ++it) {
                    result += std::to_string(it->digit + '0');
                }
                return result;
            }
        };

        number add(number& _num1, number& _num2)
        {
            number result;
            function adder = function::create_addition_function(9);
            function modulo = function::create_modulos_function(9);

            int idx = 0;
            bool ok = !_num1.empty() || !_num2.empty();
            symbol carry;
            symbol base10 = modulo.get_symbol(10);

            while (ok) 
            {
                symbol r;
                if (idx < _num1.symbols.size() && idx < _num2.symbols.size())
                {
                    symbol xp = _num1.symbols[idx];
                    symbol x = adder.calculate(xp, carry);
                    symbol y = _num2.symbols[idx];
                    r = adder.calculate(x, y);
                }
                else if (idx < _num1.symbols.size()) 
                {
                    symbol x = _num1.symbols[idx];
                    symbol y = carry;
                    r = adder.calculate(x, y);
                    result.symbols.push_back(r);
                }
                else if (idx < _num2.symbols.size()) 
                {
                    symbol x = carry;
                    symbol y = _num2.symbols[idx];
                    r = adder.calculate(x, y);
                    result.symbols.push_back(r);
                }
                if (r.digit >= 10)
                {
                    r = modulo.calculate(r, base10);
                    carry = modulo.get_symbol(1);
                }
                else
                {
                    carry = modulo.get_symbol(0);
                }
                result.symbols.push_back(r);

                idx++;
                ok = idx < _num1.symbols.size() || idx < _num2.symbols.size();
            }

            return result;
        }

        number add(int _num1, int _num2)
        {
            std::string ns1 = std::to_string(_num1);
            std::string ns2 = std::to_string(_num2);

            number n1(ns1);
            number n2(ns2);
            return add(n1, n2);
        }

        number add(std::string _num1, std::string _num2)
        {
            number n1(_num1);
            number n2(_num2);
            return add(n1, n2);
        }

        void test_addition()
        {
            for (int i = 0; i < 1000; i++) {
                for (int j = 0; j < 1000; j++) {
                    number result = add(i, j);
                    int expected = i + j;
                    std::string result_str = result.to_string();
                    std::string expected_str = std::to_string(expected);
                    if (result_str != expected_str) {
                        std::cerr << "Addition test failed: " << i << " + " << j << " = " << result_str << ", expected: " << expected_str << std::endl;
                        break;
                    }
                }
            }
        }
    }

}

