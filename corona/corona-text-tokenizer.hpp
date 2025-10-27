
//
//
//
//Thank you AI for this little gem.
//
//
//


namespace corona
{

    class text_tokenizer {
    public:
        std::vector<std::string> tokenize(const std::string& text) {
            std::vector<std::string> tokens;

            // Split on word boundaries (not just whitespace)
            std::regex word_regex(R"(\b\w+\b)");
            std::sregex_iterator iter(text.begin(), text.end(), word_regex);
            std::sregex_iterator end;

            for (; iter != end; ++iter) {
                std::string word = iter->str();

                // Normalize: lowercase, remove accents, etc.
                word = normalize_word(word);

                // Filter out stop words
                if (!is_stop_word(word) && word.length() >= min_word_length) {
                    tokens.push_back(word);

                    // Add stems/variations if needed
                    auto stems = get_word_stems(word);
                    tokens.insert(tokens.end(), stems.begin(), stems.end());
                }
            }

            return tokens;
        }

    private:
        int min_word_length = 2;
        std::set<std::string> stop_words = { "the", "and", "or", "but", "in", "on", "at", "to", "for", "of", "with", "by", "junior", "jr", "senior", "sr"};

        std::string normalize_word(const std::string& word) {
            std::string normalized = word;
            std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
            return normalized;
        }

        bool is_stop_word(const std::string& word) {
            return stop_words.count(word) > 0;
        }

        std::vector<std::string> get_word_stems(const std::string& word) {
            // Simple stemming - remove common suffixes
            std::vector<std::string> stems;
            if (word.length() > 4) {
                if (word.ends_with("ing")) stems.push_back(word.substr(0, word.length() - 3));
                if (word.ends_with("ed")) stems.push_back(word.substr(0, word.length() - 2));
                if (word.ends_with("s")) stems.push_back(word.substr(0, word.length() - 1));
            }
            return stems;
        }
    };

}