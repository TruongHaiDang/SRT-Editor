#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <QSettings>
#include <QString>
#include <QStringList>
#include <QVariant>

class Settings
{
public:
    Settings();
    Settings(const QString &organization, const QString &application);
    Settings(QSettings::Format format,
             QSettings::Scope scope,
             const QString &organization,
             const QString &application);
    ~Settings() = default;

    QVariant value(const QString &key, const QVariant &defaultValue = {}) const;
    void setValue(const QString &key, const QVariant &value);
    bool contains(const QString &key) const;
    void remove(const QString &key);
    QStringList childKeys() const;
    QStringList childGroups() const;
    void beginGroup(const QString &prefix);
    void endGroup();
    void clear();
    void sync();
    QString fileName() const;

private:
    QSettings settings_;
};

#endif // __SETTINGS_H__
