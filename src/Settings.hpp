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

    /* Returns value associated with the given key
     * Returns "NOT_FOUND" if there is no entry for that name-value pair
     */
    std::string getString(const std::string& key) const;

    /* Returns value associated with the given key
     * Returns 0 if there is no entry for that name-value pair
     */
    double getDouble(const std::string& key) const;

    /* Returns value associated with the given key
     * Returns 0 if there is no entry for that name-value pair
     */
    int getInt(const std::string& key) const;

    // Saves all name-value pairs to external file with the given name
    void saveToFile(std::string_view filename);

private:
    std::string m_filename;
    std::map<std::string, std::string> m_values;
};
