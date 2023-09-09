#ifndef FACTORY_H
#define FACTORY_H

#include <map>

#include "FactoryExc.h"

template <class Base>
class AbstractCreator
{
  public:
	AbstractCreator() {}
	virtual ~AbstractCreator() {}
	virtual Base* create() const = 0;
	virtual AbstractCreator<Base>* make_copy() const = 0;
};


template <class Derived, class Base>
class Creator : public AbstractCreator<Base>
{
  public:
	Creator() {}
	virtual ~Creator() {}
	virtual Base* create() const override { return new Derived(); }
	virtual AbstractCreator<Base>* make_copy() const override { return new Creator(); }
};


template <class Base, class Id = int>
class Factory
{
  protected:
	using FactoryMap = std::map< Id, AbstractCreator<Base>* >;
	void register_class(const Id &id, AbstractCreator<Base> *creator);

  private:
	FactoryMap _map;

  public:
	Factory() {};
	Factory(const Factory &other);
	Factory& operator=(Factory other) { swap(other); return *this; }
	virtual ~Factory();

	void swap(Factory &other) { _map.swap(other._map); }

	template <class Derived>
	void add(const Id &id);

	void remove(const Id &id);

	bool is_registered(const Id &id) const { return _map.find(id) != _map.cend(); }

	Base* create(const Id &id) const;
	
	size_t size() const { return _map.size(); }

	bool empty() const { return size() == 0; }
};


/* ---------------------------------------PROTECTED METHODS-------------------------------------- */

template <class Base, class Id>
void Factory<Base, Id>::register_class(const Id &id, AbstractCreator<Base> *creator)
{
	if (!_map.contains(id))
		_map[id] = creator;
	else
		throw FactoryExcIdConflict("Id already taken!");
}

/* ----------------------------------------PUBLIC METHODS---------------------------------------- */

template <class Base, class Id>
Factory<Base, Id>::Factory(const Factory &other)
{
	for (const auto& kv : other._map)
		_map[kv.first] = kv.second->make_copy();
}

template <class Base, class Id>
Factory<Base, Id>::~Factory()
{
	for (auto it = _map.begin(); it != _map.end(); ++it)
		delete it->second;
}


template <class Base, class Id>
template <class Derived>
void Factory<Base, Id>::add(const Id &id)
{
	AbstractCreator<Base> *creator;
	try {
		creator = new Creator<Derived, Base>;
		register_class(id, creator);
	} catch (...) {
		delete creator;
		throw;
	}
}

template <class Base, class Id>
void Factory<Base, Id>::remove(const Id &id)
{
	auto it = _map.find(id);
	if (it != _map.end()) {
		delete it->second;
		_map.erase(it);
	} else {
		throw FactoryExcNoId("Id not found!");
	}
}

template <class Base, class Id>
Base* Factory<Base, Id>::create(const Id &id) const
{
	auto it = _map.find(id);
	if (it != _map.end())
		return it->second->create();
	else
		throw FactoryExcNoId("Id not found!");
}

#endif // FACTORY_H
