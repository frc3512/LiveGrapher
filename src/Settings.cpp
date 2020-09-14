// Copyright (c) 2013-2020 FRC Team 3512. All Rights Reserved.

#include "Settings.hpp"

#include <fstream>
#include <iostream>
#include <regex>

Settings::Settings(std::string_view filename) : m_filename(filename) {
    update();
}

void Settings::update() {
    m_values.clear();

    std::ifstream settings{m_filename};
    if (!settings.is_open()) {
        std::cerr << "Failed to open " << m_filename << "\n";
        return;
    }

    std::string line;
    std::regex lineRegex{"(\\w+)\\s*=\\s*([\\w\\.]+)"};
    while (std::getline(settings, line)) {
        std::smatch match;
        if (std::regex_search(line, match, lineRegex)) {
            m_values[match.str(1)] = match.str(2);
        }
    }
}

std::string Settings::getString(const std::string& key) const {
    auto index = m_values.find(key);

    // If the element wasn't found
    if (index == m_values.end()) {
        std::cerr << "Settings: " << m_filename << ": '" << key
                  << "' not found\n";
        return "NOT_FOUND";
    }

    // Else return the value for that element
    return index->second;
}

double Settings::getDouble(const std::string& key) const {
    auto index = m_values.find(key);

    // If the element wasn't found
    if (index == m_values.end()) {
        std::cerr << "Settings: " << m_filename << ": '" << key
                  << "' not found\n";
        return 0.f;
    }

    // Else return the value for that element
    return std::stof(index->second);
}

int Settings::getInt(const std::string& key) const {
    auto index = m_values.find(key);

    // If the element wasn't found
    if (index == m_values.end()) {
        std::cerr << "Settings: " << m_filename << ": '" << key
                  << "' not found\n";
        return 0;
    }

    // Else return the value for that element
    return std::stoi(index->second);
}

void Settings::saveToFile(std::string_view filename) {
    std::ofstream outFile(std::string{filename},
                          std::ostream::out | std::ostream::trunc);

    if (outFile.is_open()) {
        for (auto index : m_values) {
            outFile << index.first << " = " << index.second << "\n";
        }
    }
}
