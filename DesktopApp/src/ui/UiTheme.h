#ifndef UITHEME_H
#define UITHEME_H

#include <QString>

/** Applies the shared, high-DPI-safe workstation theme at application scope. */
class UiTheme {
public:
    static void apply(const QString& themeName);
};

#endif // UITHEME_H
