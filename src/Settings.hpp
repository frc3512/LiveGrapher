// Copyright (c) 2013-2020 FRC Team 3512. All Rights Reserved.

#pragma once

#include <map>
#include <string>
#include <string_view>

/**
 * Opens a given file and creates a map of its key-value pairs.
 */
class Settings {
public:
    explicit Settings(std::string_view filename);

    /**
     * Updates list of values from given file.
     */
    void update();

    /**
     * Returns value associated with the given key or "NOT_FOUND" if one doesn't
     * exist.
     *
     * @param key The key.
     */
    std::string getString(const std::string& key) const;

    /**
     * Returns value associated with the given key or 0 if one doesn't exist.
     *
     * @param key The key.
     */
    double getDouble(const std::string& key) const;

    /**
     * Returns value associated with the given key or 0 if one doesn't exist.
     *
     * @param key The key.
     */
    int getInt(const std::string& key) const;

    /**
     * Saves all name-value pairs to a file with the given name.
     *
     * @param filename Name of the file.
     */
    void saveToFile(std::string_view filename);

private:
    std::string m_filename;
    std::map<std::string, std::string> m_values;
};
