#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "debug_alloc.h"
#include "Log.h"
#include "Config.h"

using namespace MyTools;

static const char *MYTAG = "Config";

struct MyTools::ConfigEntry
{
	LString key;
	LString val;
	ConfigEntry *next;
};

// ConfigGroup members

inline ConfigEntry *ConfigGroup::findEntry(const LString &key) const
{
	ConfigEntry *pent = _child;
	while (pent != NULL && pent->key != key)
		pent = pent->next;
	return pent;
}

ConfigEntry *ConfigGroup::addEntry(const LString &key, const LString &value)
{
	ConfigEntry *ent = NEW ConfigEntry;
	if (ent == NULL)
		return NULL;

	ent->key = key.simplified();
	ent->val = value.simplified();
	ent->next = NULL;

	if (_lastEntry == NULL) {
		assert(_child == NULL);
		_child = _lastEntry = ent;
	}
	else {
		_lastEntry->next = ent;
		_lastEntry = ent;
	}

	return ent;
}

bool ConfigGroup::save(FILE *fp)
{
	assert(fp != NULL);

	clearerr(fp);

	fputc('[', fp);
	fputs(_groupName.toUtf8(), fp);
	fputs("]\r\n", fp);

	ConfigEntry *pent = _child;
	for (; pent != NULL && !ferror(fp); pent = pent->next) {
		if (!pent->key.isEmpty()) {
			fputs(pent->key.toUtf8(), fp);
			fputc('=', fp);
			if (!pent->val.isEmpty())
				fputs(pent->val.toUtf8(), fp);
			fputs("\r\n", fp);
		}
	}

	return !ferror(fp);
}

ConfigGroup::ConfigGroup()
{
	_sibling = NULL;
	_child = NULL;
	_lastEntry = NULL;
}

ConfigGroup::ConfigGroup(const LString &name)
{
	_sibling = NULL;
	_groupName = name.simplified();
	_child = NULL;
	_lastEntry = NULL;
}

ConfigGroup::ConfigGroup(const ConfigGroup &grp)
{
	_sibling = NULL;
	_groupName = grp.groupName();
	_child = grp._child;
	_lastEntry = grp._lastEntry;

	ConfigGroup *pgrp = const_cast<ConfigGroup *>(&grp);
	pgrp->_child = NULL;
	pgrp->_lastEntry = NULL;
}

ConfigGroup::~ConfigGroup()
{
	ConfigEntry *pent = _child;
	ConfigEntry *next = NULL;
	while (pent != NULL) {
		next = pent->next;
		DELETE(pent);
		pent = next;
	}
}

ConfigGroup ConfigGroup::fromString(const LString &str, char sepChar)
{
	ConfigGroup res;
	int posFieldBegin = 0;
	int posFieldEnd = str.indexOf(sepChar, posFieldBegin);
	if (posFieldEnd == -1) {
		res._groupName = str;
		return res;
	}
	res._groupName = str.mid(0, posFieldEnd - posFieldBegin);
	posFieldBegin = posFieldEnd + 1;

	while ((posFieldEnd = str.indexOf(sepChar, posFieldBegin)) != -1) {
		int posEqual = str.indexOf('=', posFieldBegin);
		if (posEqual == -1)
			return res;
		res.addEntry(str.mid(posFieldBegin, posEqual - posFieldBegin), 
				str.mid(posEqual + 1, posFieldEnd - posEqual - 1));
		posFieldBegin = posFieldEnd + 1;
	}

	if (posFieldBegin < static_cast<int>(str.length())) {
		int posEqual = str.indexOf('=', posFieldBegin);
		if (posEqual != -1)
			res.addEntry(str.mid(posFieldBegin, posEqual - posFieldBegin), str.mid(posEqual + 1));
	}

	return res;
}

bool ConfigGroup::hasKey(const LString &key)
{
	return findEntry(key) != NULL;
}

void ConfigGroup::setValue(const LString &key, const LString &value)
{
	ConfigEntry *pent = findEntry(key);
	if (pent != NULL)
		pent->val = value.simplified();
	else
		addEntry(key, value);
}

void ConfigGroup::setValue(const LString &key, long value)
{
	ConfigEntry *pent = findEntry(key);
	if (pent != NULL)
		pent->val.setNumber(value);
	else
		addEntry(key, LString::number(value));
}

void ConfigGroup::setValue(const LString &key, unsigned long value)
{
	ConfigEntry *pent = findEntry(key);
	if (pent != NULL)
		pent->val.setNumber(value);
	else
		addEntry(key, LString::number(value));
}

void ConfigGroup::setValue(const LString &key, double value)
{
	ConfigEntry *pent = findEntry(key);
	if (pent != NULL)
		pent->val.setNumber(value);
	else
		addEntry(key, LString::number(value));
}

LString ConfigGroup::getValue(const LString &key, const LString &defaultValue) const
{
	ConfigEntry *pent = findEntry(key);
	if (pent != NULL)
		return pent->val;
	else {
		LOG_E(MYTAG, "no such key: " + key);
		return defaultValue;
	}
}

long ConfigGroup::getIntValue(const LString &key, const long defaultValue) const
{
	ConfigEntry *pent = findEntry(key);
	if (pent == NULL) {
		LOG_E(MYTAG, "no such key: " + key);
		return defaultValue;
	}

	bool ok = false;
	long res = pent->val.toLong(&ok);
	if (!ok) {
		LOG_E(MYTAG, "invalid INT entry: " + pent->key + "=" + pent->val);
		res = defaultValue;
	}
	return res;
}

unsigned long ConfigGroup::getUIntValue(const LString &key, unsigned long defaultValue) const
{
	ConfigEntry *pent = findEntry(key);
	if (pent == NULL) {
		LOG_E(MYTAG, "no such key: " + key);
		return defaultValue;
	}

	bool ok = false;
	unsigned long res = pent->val.toULong(&ok);
	if (!ok) {
		LOG_E(MYTAG, "invalid UINT entry: " + pent->key + "=" + pent->val);
		res = defaultValue;
	}
	return res;
}

double ConfigGroup::getFloatValue(const LString &key, double defaultValue) const
{
	ConfigEntry *pent = findEntry(key);
	if (pent == NULL) {
		LOG_E(MYTAG, "no such key: " + key);
		return defaultValue;
	}

	bool ok = false;
	double res = pent->val.toDouble(&ok);
	if (!ok) {
		LOG_E(MYTAG, "invalid FLOAT entry: " + pent->key + "=" + pent->val);
		res = defaultValue;
	}
	return res;
}

LString ConfigGroup::toString(char sepChar) const
{
	LString s;
	s.append(_groupName);
	ConfigEntry *pent = _child;
	for (; pent != NULL; pent = pent->next) {
		s.append(sepChar);
		s.append(pent->key);
		s.append('=');
		s.append(pent->val);
	}
	return s;
}

// Config members

void Config::init()
{
	_grps = NULL;
	_curGroup = NULL;
	_dirty = false;
}

inline void Config::setEntry(const char *key, const char *val)
{
	if (_curGroup != NULL)
		_curGroup->setValue(LString::fromUtf8(key), LString::fromUtf8(val));
}

Config::Config()
{
	init();
}

Config::Config(const LString &fileName)
{
	init();
	open(fileName);
}

Config::~Config()
{
	if (_dirty)
		save();

	ConfigGroup *pgrp = _grps;
	ConfigGroup *next = NULL;
	for (; pgrp != NULL; pgrp = next) {
		next = pgrp->_sibling;
		DELETE(pgrp);
	}
}

bool Config::open(const LString &fileName)
{
	_fileName = fileName;

	FILE *fp = fopen(fileName.toUtf8(), "r");
	if (fp == NULL) {
		LOG_E(MYTAG, "Can not open file: " + fileName);
		return false;
	}

	char *line = NEW char[1024];
	if (line == NULL) {
		fclose(fp);
		return false;
	}

	char *p = NULL;
	while ((p = fgets(line, 1023, fp)) != NULL) {
		if (*p == '[') { // group name
			char *grpnm = p + 1;
			for (; *p != '\0' && *p != ']'; ++p) ; // finding ']'
			if (*p == ']') {
				*p = '\0';
				setGroup(grpnm);
			}
		}
		else if (*p != '#') {
			const char *start = p;
			const char *key = NULL;
			const char *val = NULL;
			for (; *p != '\0' && *p != '='; ++p) ; // finding '='
			if (*p == '=') {
				key = start;
				val = p + 1;
				*p = '\0';
				setEntry(key, val);
			}
		}
	}

	DELETE_ARR(line);
	fclose(fp);

	return true;
}

bool Config::save()
{
	FILE *fp = fopen(_fileName.toUtf8(), "w");
	if (fp == NULL) {
		LOG_E(MYTAG, "Can not open file: " + _fileName);
		return false;
	}

	ConfigGroup *pgrp = _grps;
	bool ok = true;
	for (; pgrp != NULL && ok; pgrp = pgrp->_sibling)
		ok = pgrp->save(fp);

	_dirty = !ok;

	fclose(fp);
	return ok;
}

bool Config::hasGroup(const LString &name)
{
	ConfigGroup *pgrp = _grps;
	for (; pgrp != NULL; pgrp = pgrp->_sibling)
		if (pgrp->groupName() == name)
			break;
	return pgrp != NULL;
}

void Config::setGroup(const LString &name)
{
	ConfigGroup *pgrp = _grps;
	ConfigGroup **after = &_grps;
	while (pgrp != NULL) {
		if (pgrp->groupName() == name)
			break;
		after = &(pgrp->_sibling);
		pgrp = pgrp->_sibling;
	}

	if (pgrp == NULL)
		*after = pgrp = NEW ConfigGroup(name);

	_curGroup = pgrp;
}

void Config::removeGroup(const LString &name)
{
	ConfigGroup *pgrp = _grps;
	ConfigGroup **after = &_grps;
	while (pgrp != NULL) {
		if (pgrp->groupName() == name)
			break;
		after = &(pgrp->_sibling);
		pgrp = pgrp->_sibling;
	}

	if (pgrp == NULL) {
		LOG_E(MYTAG, "group not exists: " + name);
		return;
	}

	if (_curGroup == pgrp)
		_curGroup = NULL;

	*after = pgrp->_sibling;
	DELETE(pgrp);

	_dirty = true;
}

void Config::setValue(const LString &key, const LString &val)
{
	if (_curGroup != NULL) {
		_curGroup->setValue(key, val);
		_dirty = true;
	}
}

void Config::setValue(const LString &key, int val)
{
	setValue(key, static_cast<long>(val));
}

void Config::setValue(const LString &key, unsigned int val)
{
	setValue(key, static_cast<unsigned long>(val));
}

void Config::setValue(const LString &key, long val)
{
	if (_curGroup != NULL) {
		_curGroup->setValue(key, val);
		_dirty = true;
	}
}

void Config::setValue(const LString &key, unsigned long val)
{
	if (_curGroup != NULL) {
		_curGroup->setValue(key, val);
		_dirty = true;
	}
}

void Config::setValue(const LString &key, double val)
{
	if (_curGroup != NULL) {
		_curGroup->setValue(key, val);
		_dirty = true;
	}
}

LString Config::getValue(const LString &key, const LString &defaultValue) const
{
	if (_curGroup != NULL)
		return _curGroup->getValue(key, defaultValue);
	else {
		LOG_E(MYTAG, "no current group, default value returned");
		return defaultValue;
	}
}

long Config::getIntValue(const LString &key, long defaultValue) const
{
	if (_curGroup != NULL)
		return _curGroup->getIntValue(key, defaultValue);
	else {
		LOG_E(MYTAG, "no current group, default INT value returned");
		return defaultValue;
	}
}

unsigned long Config::getUIntValue(const LString &key, unsigned long defaultValue) const
{
	if (_curGroup != NULL)
		return _curGroup->getUIntValue(key, defaultValue);
	else {
		LOG_E(MYTAG, "no current group, default UINT value returned");
		return defaultValue;
	}
}

double Config::getFloatValue(const LString &key, double defaultValue) const
{
	if (_curGroup != NULL)
		return _curGroup->getFloatValue(key, defaultValue);
	else {
		LOG_E(MYTAG, "no current group, default FLOAT value returned");
		return defaultValue;
	}
}

LString Config::toString(char sepChar) const
{
	LString s;
	if (_curGroup != NULL)
		s = _curGroup->toString(sepChar);
	return s;
}

Config::GroupIterator Config::begin()
{
	return GroupIterator(_grps);
}

Config::GroupIterator Config::end()
{
	return GroupIterator(NULL);
}

// Config::GroupIterator members

Config::GroupIterator &Config::GroupIterator::operator++()
{ 
	_p = *reinterpret_cast<ConfigGroup **>(_p); // _p = _p->_sibling;
	return *this; 
}

