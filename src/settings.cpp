#include "settings.h"

Settings::Settings()
    : settings_()
{
}

Settings::Settings(const QString &organization, const QString &application)
    : settings_(organization, application)
{
}

Settings::Settings(QSettings::Format format,
                   QSettings::Scope scope,
                   const QString &organization,
                   const QString &application)
    : settings_(format, scope, organization, application)
{
}

QVariant Settings::value(const QString &key, const QVariant &defaultValue) const
{
    return settings_.value(key, defaultValue);
}

void Settings::setValue(const QString &key, const QVariant &value)
{
    settings_.setValue(key, value);
}

bool Settings::contains(const QString &key) const
{
    return settings_.contains(key);
}

void Settings::remove(const QString &key)
{
    settings_.remove(key);
}

QStringList Settings::childKeys() const
{
    return settings_.childKeys();
}

QStringList Settings::childGroups() const
{
    return settings_.childGroups();
}

void Settings::beginGroup(const QString &prefix)
{
    settings_.beginGroup(prefix);
}

void Settings::endGroup()
{
    settings_.endGroup();
}

void Settings::clear()
{
    settings_.clear();
}

void Settings::sync()
{
    settings_.sync();
}

QString Settings::fileName() const
{
    return settings_.fileName();
}
