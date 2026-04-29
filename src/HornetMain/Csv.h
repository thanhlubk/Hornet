#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <stdexcept>

class CsvTable {
public:
    explicit CsvTable(const std::string& path) {
        std::ifstream in(path);
        if (!in) {
            throw std::runtime_error("Failed to open CSV: " + path);
        }
        std::string line;
        if (!std::getline(in, line)) {
            throw std::runtime_error("Empty CSV: " + path);
        }
        headers_ = split(line);
        for (std::size_t i = 0; i < headers_.size(); ++i) {
            index_[trim(headers_[i])] = i;
        }
        while (std::getline(in, line)) {
            if (trim(line).empty()) {
                continue;
            }
            rows_.push_back(split(line));
        }
    }

    std::size_t rowCount() const noexcept { return rows_.size(); }

    double getDouble(std::size_t row, const std::string& col) const {
        return std::stod(getString(row, col));
    }

    int getInt(std::size_t row, const std::string& col) const {
        return static_cast<int>(std::llround(std::stod(getString(row, col))));
    }

    std::string getString(std::size_t row, const std::string& col) const {
        const auto it = index_.find(col);
        if (it == index_.end()) {
            throw std::runtime_error("CSV column not found: " + col);
        }
        if (row >= rows_.size() || it->second >= rows_[row].size()) {
            throw std::runtime_error("CSV access out of range");
        }
        return trim(rows_[row][it->second]);
    }

private:
    static std::vector<std::string> split(const std::string& line) {
        std::vector<std::string> out;
        std::string cur;
        bool inQuotes = false;
        for (char ch : line) {
            if (ch == '"') {
                inQuotes = !inQuotes;
            } else if (ch == ',' && !inQuotes) {
                out.push_back(cur);
                cur.clear();
            } else {
                cur.push_back(ch);
            }
        }
        out.push_back(cur);
        return out;
    }

    static inline std::string trim(std::string s)
    {
        auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
        s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
        return s;
    }

    std::vector<std::string> headers_;
    std::unordered_map<std::string, std::size_t> index_;
    std::vector<std::vector<std::string>> rows_;
};