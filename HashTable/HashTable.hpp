#ifndef HASH_TABLE_HPP
#define HASH_TABLE_HPP

#define HASH_GROUPS 1'000ull	// количество возможных хеш-значений по умолчанию

#include <functional>
#include <forward_list>

#include "HashTableExc.h"

template< class Key, class T, class Hash = std::hash<Key> >
class HashTable
{
private:
	typedef std::forward_list< std::pair<Key, T> > HashClass;

	size_t _size;
	size_t _hashes;		// хеш-значения будут из промежутка [0, _hashes)
	HashClass *_table;	// классы эквивалентности по соответсвующим хеш-значениям

	size_t get_hash(const Key &key) const { return Hash{}(key) % _hashes; }

  public:
	class Iterator;
	class ConstIterator;

	HashTable(size_t hashes = HASH_GROUPS) :
		_size(0), _hashes(hashes), _table(new HashClass[_hashes]) {}
	HashTable(const HashTable &other);
	HashTable& operator=(HashTable other) { swap(other); return *this; }

	~HashTable() { delete[] _table; }
	void clear();

	bool empty() const { return _size == 0; }
	size_t size() const { return _size; }

	void swap(HashTable &other);

	Iterator begin() { return Iterator(this, 0, _table[0].begin()); }
	ConstIterator cbegin() const { return ConstIterator(this, 0, _table[0].cbegin()); }
	Iterator end() { return Iterator(this, _hashes - 1, _table[_hashes - 1].end()); }
	ConstIterator cend() const {
		return ConstIterator(this, _hashes - 1, _table[_hashes - 1].cend());
	}

	T& operator[](const Key &key);
	const T& operator[](const Key &key) const;
	Iterator find(const Key &key);
	ConstIterator find(const Key &key) const;
	size_t erase(const Key &key);
	size_t count(const Key &key) const { return (find(key) == cend()) ? 0 : 1; }
};

/* --------------------------------------------ITERATORS----------------------------------------- */

template <class Key, class T, class Hash>
class HashTable<Key, T, Hash>::Iterator
{
	friend HashTable;

	HashTable *_ht;
	size_t _hash;
	typename HashClass::iterator _it;

	Iterator(HashTable *ht, size_t hash, typename HashClass::iterator it) : 
		_ht(ht), _hash(hash), _it(it) {}

public:
	Iterator() : _ht(nullptr) {}
	Iterator(const Iterator &other) : _ht(other._ht), _hash(other._hash), _it(other._it) {}
	Iterator& operator=(const Iterator &other)
		{ _ht = other._ht; _hash = other._hash; _it = other._it; return *this; }
	Iterator& operator++();
	Iterator operator++(int) { Iterator it = *this; ++(*this); return it; }
	const Key& key() const { return _it->first; }
	T& val() const { return _it->second; }
	bool operator==(const HashTable::Iterator &other) const;
	bool operator!=(const HashTable::Iterator &other) { return !(*this == other); }
};

template <class Key, class T, class Hash>
class HashTable<Key, T, Hash>::ConstIterator
{
	friend HashTable;

	const HashTable *_ht;
	size_t _hash;
	typename HashClass::const_iterator _it;

	ConstIterator(const HashTable *ht, size_t hash, typename HashClass::const_iterator it) :
		_ht(ht), _hash(hash), _it(it) {}

  public:
	ConstIterator() : _ht(nullptr) {}
	ConstIterator(const Iterator &other) : _ht(other._ht), _hash(other._hash), _it(other._it) {}
	ConstIterator &operator=(const Iterator &other) { 
		_ht = other._ht; _hash = other._hash; _it = other._it; return *this;
	}
	ConstIterator& operator++();
	ConstIterator operator++(int) { ConstIterator it = *this; ++(*this); return it; }
	const Key& key() const { return _it->first; }
	const T& val() const { return _it->second; }
	friend bool operator==(const HashTable::ConstIterator &a, const HashTable::ConstIterator &b) {
		if (a._ht != b._ht)
			return false;
		if (a._ht->empty())
			return true;
		return a._hash == b._hash && a._it == b._it;
	}
	friend bool operator!=(const HashTable::ConstIterator &a, const HashTable::ConstIterator &b) {
		return !(a == b);
	}
};

template <class Key, class T, class Hash>
bool HashTable<Key, T, Hash>::Iterator::operator==(const HashTable::Iterator &other) const
{
	if (_ht != other._ht)
		return false;
	if (_ht->empty())
		return true;
	return _hash == other._hash && _it == other._it;
}

template <class Key, class T, class Hash>
typename HashTable<Key, T, Hash>::Iterator& 
HashTable<Key, T, Hash>::Iterator::operator++()
{
	++_it;
	while (_it == _ht->_table[_hash].end()) {
		if (_hash + 1 == _ht->_hashes)
			break;
		_it = _ht->_table[++_hash].begin();
	}
	return *this;
}

template <class Key, class T, class Hash>
typename HashTable<Key, T, Hash>::ConstIterator& 
HashTable<Key, T, Hash>::ConstIterator::operator++()
{
	++_it;
	while (_it == _ht->_table[_hash].cend()) {
		if (_hash + 1 == _ht->_hashes)
			break;
		_it = _ht->_table[++_hash].cbegin();
	}
	return *this;
}

/* -----------------------------------------PUBLIC METHODS--------------------------------------- */

template <class Key, class T, class Hash>
HashTable<Key, T, Hash>::HashTable(const HashTable &other) :
_size(other._size), _hashes(other._hashes), _table(new HashClass[_hashes])
{
	for (size_t i = 0; i < _hashes; i++)
		_table[i] = other._table[i];
}

template <class Key, class T, class Hash>
void HashTable<Key, T, Hash>::clear() {
	for (int i = 0; i < _hashes; ++i)
		_table[i].clear();
	_size = 0;
}

template <class Key, class T, class Hash>
void HashTable<Key, T, Hash>::swap(HashTable &other)
{
	std::swap(_size, other._size);
	std::swap(_hashes, other._hashes);
	std::swap(_table, other._table);
}

template <class Key, class T, class Hash>
T& HashTable<Key, T, Hash>::operator[](const Key &key)
{
	size_t h = get_hash(key);
	auto it = find(key);
	if (it != end())
		return it.val();
	_table[h].push_front({key, T()});
	++_size;
	return _table[h].front().second;
}

template <class Key, class T, class Hash>
const T& HashTable<Key, T, Hash>::operator[](const Key &key) const
{
	ConstIterator it = find(key);
	if (it == cend())
		throw HashTableExcKey("No such key in the immutable HashTable!");
	return it.val();
}

template<class Key, class T, class Hash>
typename HashTable<Key, T, Hash>::Iterator
HashTable<Key, T, Hash>::find(const Key &key)
{
	size_t h = get_hash(key);
	auto cur_it = _table[h].begin();
	while (cur_it != _table[h].end()) {
		if (cur_it->first == key)
			return Iterator(this, h, cur_it);
		++cur_it;
	}
	return end();
}

template<class Key, class T, class Hash>
typename HashTable<Key, T, Hash>::ConstIterator
HashTable<Key, T, Hash>::find(const Key &key) const
{
	size_t h = get_hash(key);
	auto cur_it = _table[h].cbegin();
	while (cur_it != _table[h].cend()) {
		if (cur_it->first == key)
			return ConstIterator(this, h, cur_it);
		++cur_it;
	}
	return cend();
}

template<class Key, class T, class Hash>
size_t HashTable<Key, T, Hash>::erase(const Key &key)
{
	size_t h = get_hash(key);
	auto prev_it = _table[h].before_begin();
	auto cur_it = _table[h].begin();
	while (cur_it != _table[h].end()) {
		if (cur_it->first == key) {
			_table[h].erase_after(prev_it);
			--_size;
			return 1;
		}
		prev_it = cur_it;
		++cur_it;
	}
	return 0;
}

#endif // HASH_TABLE_HPP
