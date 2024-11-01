#ifndef MYTOOLS_CONFIG_H
#define MYTOOLS_CONFIG_H

#include <stdio.h>
#include "tools_gloabal.h"

#include "LString.h"

namespace MyTools {
struct ConfigEntry;

class TOOLS_EXPORT ConfigGroup
{
private:
    ConfigGroup * _sibling; // Makes the sibling pointer be first is intentional.
    LString       _groupName;
    ConfigEntry * _child;
    ConfigEntry * _lastEntry;

    static const char DEFAULT_SEPCHAR = ':';

private:
    ConfigEntry * findEntry(const LString & key) const;
    ConfigEntry * addEntry(const LString & key, const LString & value);

    bool save(FILE * fp);

private:
    ConfigGroup & operator=(const ConfigGroup &);

public:
    ConfigGroup();
    ConfigGroup(const LString & name);
    ConfigGroup(const ConfigGroup &); // relay mode copy
    ~ConfigGroup();

    static ConfigGroup fromString(const LString &, char sepChar = DEFAULT_SEPCHAR);

    void            setGroupName(const LString &);
    const LString & groupName() const;

    bool isEmpty() const;
    bool hasKey(const LString &);

    void setValue(const LString & key, const LString &);
    void setValue(const LString & key, int);
    void setValue(const LString & key, unsigned int);
    void setValue(const LString & key, long);
    void setValue(const LString & key, unsigned long);
    void setValue(const LString & key, double);

    LString       getValue(const LString & key, const LString & defaultValue = LString::null) const;
    long          getIntValue(const LString & key, long defaultValue = 0L) const;
    unsigned long getUIntValue(const LString & key, unsigned long defaultValue = 0UL) const;
    double        getFloatValue(const LString & key, double defaultValue = 0.0) const;

    // <GroupName><sepChar><key>=<val><sepChar>...<key>=<val>
    LString toString(char sepChar = DEFAULT_SEPCHAR) const;

    friend class Config;
};

// ConfigGroup inline functions

inline void ConfigGroup::setGroupName(const LString & str)
{
    _groupName = str;
}

inline const LString & ConfigGroup::groupName() const
{
    return _groupName;
}

inline bool ConfigGroup::isEmpty() const
{
    return _child == NULL;
}

inline void ConfigGroup::setValue(const LString & key, int v)
{
    setValue(key, static_cast<long>(v));
}

inline void ConfigGroup::setValue(const LString & key, unsigned int v)
{
    setValue(key, static_cast<unsigned long>(v));
}

// ~

class TOOLS_EXPORT Config
{
private:
    LString       _fileName;
    ConfigGroup * _grps;
    ConfigGroup * _curGroup;
    bool          _dirty;

private:
    void init();
    void setEntry(const char * key, const char * val);

    // no copys
    Config(const Config &);
    Config & operator=(const Config &);

public:
    Config();
    Config(const LString & fileName);
    ~Config();

    bool open(const LString & fileName);
    bool save();

    bool isNull() const;

    bool hasGroup(const LString & name);
    void setGroup(const LString & name);
    void removeGroup(const LString & name);

    void setValue(const LString & key, const LString &);
    void setValue(const LString & key, int);
    void setValue(const LString & key, unsigned int);
    void setValue(const LString & key, long);
    void setValue(const LString & key, unsigned long);
    void setValue(const LString & key, double);

    LString       getValue(const LString & key, const LString & defaultValue = LString::null) const;
    long          getIntValue(const LString & key, long defaultValue = 0L) const;
    unsigned long getUIntValue(const LString & key, unsigned long defaultValue = 0UL) const;
    double        getFloatValue(const LString & key, double defaultValue = 0.0) const;

    LString toString(char sepChar = ConfigGroup::DEFAULT_SEPCHAR) const;

    class GroupIterator
    {
    private:
        ConfigGroup * _p;

        // Constructor for Config
        GroupIterator(ConfigGroup *);

    public:
        bool                operator==(const GroupIterator &) const;
        bool                operator!=(const GroupIterator &) const;
        GroupIterator &     operator++();
        const ConfigGroup * operator->() const;

        friend class Config;
    };

    GroupIterator begin();
    GroupIterator end();
};

// Config inline functions

inline bool Config::isNull() const
{
    return _grps == NULL;
}

// Config::GroupIterator inline functions

inline Config::GroupIterator::GroupIterator(ConfigGroup * p)
{
    _p = p;
}

inline bool Config::GroupIterator::operator==(const GroupIterator & it) const
{
    return _p == it._p;
}

inline bool Config::GroupIterator::operator!=(const GroupIterator & it) const
{
    return _p != it._p;
}

inline const ConfigGroup * Config::GroupIterator::operator->() const
{
    return _p;
}

// ~
}; // namespace MyTools

#endif
