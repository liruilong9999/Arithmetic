#ifndef MYTOOLS_CONFIG_H
#define MYTOOLS_CONFIG_H

#include <stdio.h>

#include "String.h"

namespace MyTools
{
	struct ConfigEntry;

	class ConfigGroup
	{
	private:
		ConfigGroup *_sibling; // Makes the sibling pointer be first is intentional.
		String _groupName;
		ConfigEntry *_child;
		ConfigEntry *_lastEntry;

		static const char DEFAULT_SEPCHAR = ':';

	private:
		ConfigEntry *findEntry(const String &key) const;
		ConfigEntry *addEntry(const String &key, const String &value);

		bool save(FILE *fp);

	private:
		ConfigGroup &operator=(const ConfigGroup &);

	public:
		ConfigGroup();
		ConfigGroup(const String &name);
		ConfigGroup(const ConfigGroup &); // relay mode copy
		~ConfigGroup();

		static ConfigGroup fromString(const String &, char sepChar = DEFAULT_SEPCHAR);

		void setGroupName(const String &);
		const String &groupName() const;

		bool isEmpty() const;
		bool hasKey(const String &);

		void setValue(const String &key, const String &);
		void setValue(const String &key, int);
		void setValue(const String &key, unsigned int);
		void setValue(const String &key, long);
		void setValue(const String &key, unsigned long);
		void setValue(const String &key, double);

		String getValue(const String &key, const String &defaultValue = String::null) const;
		long getIntValue(const String &key, long defaultValue = 0L) const;
		unsigned long getUIntValue(const String &key, unsigned long defaultValue = 0UL) const;
		double getFloatValue(const String &key, double defaultValue = 0.0) const;

		// <GroupName><sepChar><key>=<val><sepChar>...<key>=<val>
		String toString(char sepChar = DEFAULT_SEPCHAR) const;

		friend class Config;
	};

	// ConfigGroup inline functions

	inline void ConfigGroup::setGroupName(const String &str)
	{ _groupName = str; }

	inline const String &ConfigGroup::groupName() const
	{ return _groupName; }

	inline bool ConfigGroup::isEmpty() const
	{ return _child == NULL; }

	inline void ConfigGroup::setValue(const String &key, int v)
	{ setValue(key, static_cast<long>(v)); }

	inline void ConfigGroup::setValue(const String &key, unsigned int v)
	{ setValue(key, static_cast<unsigned long>(v)); }

	// ~

	class Config
	{
	private:
		String _fileName;
		ConfigGroup *_grps;
		ConfigGroup *_curGroup;
		bool _dirty;

	private:
		void init();
		void setEntry(const char *key, const char *val);

		// no copys
		Config(const Config &);
		Config &operator=(const Config &);

	public:
		Config();
		Config(const String &fileName);
		~Config();

		bool open(const String &fileName);
		bool save();

		bool isNull() const;

		bool hasGroup(const String &name);
		void setGroup(const String &name);
		void removeGroup(const String &name);

		void setValue(const String &key, const String &);
		void setValue(const String &key, int);
		void setValue(const String &key, unsigned int);
		void setValue(const String &key, long);
		void setValue(const String &key, unsigned long);
		void setValue(const String &key, double);

		String getValue(const String &key, const String &defaultValue = String::null) const;
		long getIntValue(const String &key, long defaultValue = 0L) const;
		unsigned long getUIntValue(const String &key, unsigned long defaultValue = 0UL) const;
		double getFloatValue(const String &key, double defaultValue = 0.0) const;

		String toString(char sepChar = ConfigGroup::DEFAULT_SEPCHAR) const;

		class GroupIterator
		{
		private:
			ConfigGroup *_p;

			// Constructor for Config
			GroupIterator(ConfigGroup *);

		public:
			bool operator==(const GroupIterator &) const;
			bool operator!=(const GroupIterator &) const;
			GroupIterator &operator++();
			const ConfigGroup *operator->() const;

			friend class Config;
		};

		GroupIterator begin();
		GroupIterator end();
	};

	// Config inline functions

	inline bool Config::isNull() const
	{ return _grps == NULL; }

	// Config::GroupIterator inline functions

	inline Config::GroupIterator::GroupIterator(ConfigGroup *p)
	{ _p = p; }

	inline bool Config::GroupIterator::operator==(const GroupIterator &it) const
	{ return _p == it._p; }

	inline bool Config::GroupIterator::operator!=(const GroupIterator &it) const
	{ return _p != it._p; }

	inline const ConfigGroup *Config::GroupIterator::operator->() const
	{ return _p; }
	
	// ~
};

#endif
