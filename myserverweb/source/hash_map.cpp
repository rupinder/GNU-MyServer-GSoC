/*
MyServer
Copyright © 2005 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*
The superFastHash hash function is authored by Paul Hsieh
(http://www.azillionmonkeys.com/qed/bio.html)

It's released under the Paul Hsieh derivative license
(http://www.azillionmonkeys.com/qed/weblicense.html)
*/

#define HASHMAP_CPP
#include "../include/hash_map.h"

//HashMap
template <typename KeyType, typename ValueType>
HashMap<KeyType, ValueType>::HashMap()
{
	power=4;
	capacity=16;
	mask=0xffffffff>>(32-4);
	highLoadFactor=0.75f;
	lowLoadFactor=0.75f/13*6;

	load=0;
	offset=1;

	data.resize(16);
	map.resize(16, 0);
}

template <typename ValueType>
HashMap<string, ValueType>::HashMap()
{
	power=4;
	capacity=16;
	mask=0xffffffff>>(32-4);
	highLoadFactor=0.75f;
	lowLoadFactor=0.75f/13*6;

	load=0;
	offset=1;

	data.resize(16);
	map.resize(16, 0);
}

template <typename ValueType>
HashMap<char*, ValueType>::HashMap()
{
	power=4;
	capacity=16;
	mask=0xffffffff>>(32-4);
	highLoadFactor=0.75f;
	lowLoadFactor=0.75f/13*6;

	load=0;
	offset=1;

	data.resize(16);
	map.resize(16, 0);
}

template <typename ValueType>
HashMap<void*, ValueType>::HashMap()
{
	power=4;
	capacity=16;
	mask=0xffffffff>>(32-4);
	highLoadFactor=0.75f;
	lowLoadFactor=0.75f/13*6;

	load=0;
	offset=1;

	data.resize(16);
	map.resize(16, 0);
}

template <typename KeyType, typename ValueType>
HashMap<KeyType, ValueType>::HashMap(int hashCapacity)
{
	if(hashCapacity>0x10)
	{
		power=5;
		for(hashCapacity>>=5;hashCapacity>0;hashCapacity>>=1)
            ++power;
		capacity=0x1<<power;
		mask=0xffffffff>>(32-power);
	}
	else
	{
		power=4;
		capacity=16;
		mask=0xffffffff>>(32-4);
	}

	highLoadFactor=0.75f;
	lowLoadFactor=0.75f/13*6;
	load=0;
	offset=1;

	data.resize(capacity);
	map.resize(capacity, 0);
}

template <typename ValueType>
HashMap<string, ValueType>::HashMap(int hashCapacity)
{
	if(hashCapacity>0x10)
	{
		power=5;
		for(hashCapacity>>=5;hashCapacity>0;hashCapacity>>=1)
            ++power;
		capacity=0x1<<power;
		mask=0xffffffff>>(32-power);
	}
	else
	{
		power=4;
		capacity=16;
		mask=0xffffffff>>(32-4);
	}

	highLoadFactor=0.75f;
	lowLoadFactor=0.75f/13*6;
	load=0;
	offset=1;

	data.resize(capacity);
	map.resize(capacity, 0);
}

template <typename ValueType>
HashMap<char*, ValueType>::HashMap(int hashCapacity)
{
	if(hashCapacity>0x10)
	{
		power=5;
		for(hashCapacity>>=5;hashCapacity>0;hashCapacity>>=1)
            ++power;
		capacity=0x1<<power;
		mask=0xffffffff>>(32-power);
	}
	else
	{
		power=4;
		capacity=16;
		mask=0xffffffff>>(32-4);
	}

	highLoadFactor=0.75f;
	lowLoadFactor=0.75f/13*6;
	load=0;
	offset=1;

	data.resize(capacity);
	map.resize(capacity, 0);
}

template <typename ValueType>
HashMap<void*, ValueType>::HashMap(int hashCapacity)
{
	if(hashCapacity>0x10)
	{
		power=5;
		for(hashCapacity>>=5;hashCapacity>0;hashCapacity>>=1)
            ++power;
		capacity=0x1<<power;
		mask=0xffffffff>>(32-power);
	}
	else
	{
		power=4;
		capacity=16;
		mask=0xffffffff>>(32-4);
	}

	highLoadFactor=0.75f;
	lowLoadFactor=0.75f/13*6;
	load=0;
	offset=1;

	data.resize(capacity);
	map.resize(capacity, 0);
}

template <typename KeyType, typename ValueType>
HashMap<KeyType, ValueType>::HashMap(float hashLoadFactor)
{
	if(hashLoadFactor>0)
	{
		highLoadFactor=hashLoadFactor;
		lowLoadFactor=hashLoadFactor/13*6;
	}
	else
	{
		highLoadFactor=0.75f;
		lowLoadFactor=0.75f/13*6;
	}

	power=4;
	capacity=16;
	mask=0xffffffff>>(32-4);
	load=0;
	offset=1;

	data.resize(16);
	map.resize(16, 0);
}

template <typename ValueType>
HashMap<string, ValueType>::HashMap(float hashLoadFactor)
{
	if(hashLoadFactor>0)
	{
		highLoadFactor=hashLoadFactor;
		lowLoadFactor=hashLoadFactor/13*6;
	}
	else
	{
		highLoadFactor=0.75f;
		lowLoadFactor=0.75f/13*6;
	}

	power=4;
	capacity=16;
	mask=0xffffffff>>(32-4);
	load=0;
	offset=1;

	data.resize(16);
	map.resize(16, 0);
}

template <typename ValueType>
HashMap<char*, ValueType>::HashMap(float hashLoadFactor)
{
	if(hashLoadFactor>0)
	{
		highLoadFactor=hashLoadFactor;
		lowLoadFactor=hashLoadFactor/13*6;
	}
	else
	{
		highLoadFactor=0.75f;
		lowLoadFactor=0.75f/13*6;
	}

	power=4;
	capacity=16;
	mask=0xffffffff>>(32-4);
	load=0;
	offset=1;

	data.resize(16);
	map.resize(16, 0);
}

template <typename ValueType>
HashMap<void*, ValueType>::HashMap(float hashLoadFactor)
{
	if(hashLoadFactor>0)
	{
		highLoadFactor=hashLoadFactor;
		lowLoadFactor=hashLoadFactor/13*6;
	}
	else
	{
		highLoadFactor=0.75f;
		lowLoadFactor=0.75f/13*6;
	}

	power=4;
	capacity=16;
	mask=0xffffffff>>(32-4);
	load=0;
	offset=1;

	data.resize(16);
	map.resize(16, 0);
}

template <typename KeyType, typename ValueType>
HashMap<KeyType, ValueType>::HashMap(int hashCapacity, float hashLoadFactor)
{
	if(hashCapacity>0x10)
	{
		power=5;
		for(hashCapacity>>=5;hashCapacity>0;hashCapacity>>=1)
            ++power;
		capacity=0x1<<power;
		mask=0xffffffff>>(32-power);
	}
	else
	{
		power=4;
		capacity=16;
		mask=0xffffffff>>(32-4);
	}

	if(hashLoadFactor>0)
	{
		highLoadFactor=hashLoadFactor;
		lowLoadFactor=hashLoadFactor/13*6;
	}
	else
	{
		highLoadFactor=0.75f;
		lowLoadFactor=0.75f/13*6;
	}

	load=0;
	offset=1;

	data.resize(capacity);
	map.resize(capacity, 0);
}

template <typename ValueType>
HashMap<string, ValueType>::HashMap(int hashCapacity, float hashLoadFactor)
{
	if(hashCapacity>0x10)
	{
		power=5;
		for(hashCapacity>>=5;hashCapacity>0;hashCapacity>>=1)
            ++power;
		capacity=0x1<<power;
		mask=0xffffffff>>(32-power);
	}
	else
	{
		power=4;
		capacity=16;
		mask=0xffffffff>>(32-4);
	}

	if(hashLoadFactor>0)
	{
		highLoadFactor=hashLoadFactor;
		lowLoadFactor=hashLoadFactor/13*6;
	}
	else
	{
		highLoadFactor=0.75f;
		lowLoadFactor=0.75f/13*6;
	}

	load=0;
	offset=1;

	data.resize(capacity);
	map.resize(capacity, 0);
}

template <typename ValueType>
HashMap<char*, ValueType>::HashMap(int hashCapacity, float hashLoadFactor)
{
	if(hashCapacity>0x10)
	{
		power=5;
		for(hashCapacity>>=5;hashCapacity>0;hashCapacity>>=1)
            ++power;
		capacity=0x1<<power;
		mask=0xffffffff>>(32-power);
	}
	else
	{
		power=4;
		capacity=16;
		mask=0xffffffff>>(32-4);
	}

	if(hashLoadFactor>0)
	{
		highLoadFactor=hashLoadFactor;
		lowLoadFactor=hashLoadFactor/13*6;
	}
	else
	{
		highLoadFactor=0.75f;
		lowLoadFactor=0.75f/13*6;
	}

	load=0;
	offset=1;

	data.resize(capacity);
	map.resize(capacity, 0);
}

template <typename ValueType>
HashMap<void*, ValueType>::HashMap(int hashCapacity, float hashLoadFactor)
{
	if(hashCapacity>0x10)
	{
		power=5;
		for(hashCapacity>>=5;hashCapacity>0;hashCapacity>>=1)
            ++power;
		capacity=0x1<<power;
		mask=0xffffffff>>(32-power);
	}
	else
	{
		power=4;
		capacity=16;
		mask=0xffffffff>>(32-4);
	}

	if(hashLoadFactor>0)
	{
		highLoadFactor=hashLoadFactor;
		lowLoadFactor=hashLoadFactor/13*6;
	}
	else
	{
		highLoadFactor=0.75f;
		lowLoadFactor=0.75f/13*6;
	}

	load=0;
	offset=1;

	data.resize(capacity);
	map.resize(capacity, 0);
}

template <typename KeyType, typename ValueType>
inline bool HashMap<KeyType, ValueType>::empty(void) const
{
	return(!load);
}

template <typename ValueType>
inline bool HashMap<string, ValueType>::empty(void) const
{
	return(!load);
}

template <typename ValueType>
inline bool HashMap<char*, ValueType>::empty(void) const
{
	return(!load);
}

template <typename ValueType>
inline bool HashMap<void*, ValueType>::empty(void) const
{
	return(!load);
}

template <typename KeyType, typename ValueType>
void HashMap<KeyType, ValueType>::clear(void)
{
	load=0;
	++offset;
	if(offset==0x0)
	{
		offset=1;
		map.assign(capacity, 0);
	}
}

template <typename ValueType>
void HashMap<string, ValueType>::clear(void)
{
	load=0;
	++offset;
	if(offset==0x0)
	{
		offset=1;
		map.assign(capacity, 0);
	}
}

template <typename ValueType>
void HashMap<char*, ValueType>::clear(void)
{
	load=0;
	++offset;
	if(offset==0x0)
	{
		offset=1;
		map.assign(capacity, 0);
	}
}

template <typename ValueType>
void HashMap<void*, ValueType>::clear(void)
{
	load=0;
	++offset;
	if(offset==0x0)
	{
		offset=1;
		map.assign(capacity, 0);
	}
}

template <typename KeyType, typename ValueType>
inline int HashMap<KeyType, ValueType>::size(void) const
{
	return(load);
}

template <typename ValueType>
inline int HashMap<string, ValueType>::size(void) const
{
	return(load);
}

template <typename ValueType>
inline int HashMap<char*, ValueType>::size(void) const
{
	return(load);
}

template <typename ValueType>
inline int HashMap<void*, ValueType>::size(void) const
{
	return(load);
}

template <typename KeyType, typename ValueType>
inline typename HashMap<KeyType, ValueType>::Iterator HashMap<KeyType, ValueType>::begin(void)
{
	tempIterator.vectorPtr=&data;
	tempIterator.offset=offset;
	if(load)
	{
		int i=0;
		for(;i<capacity;++i)
		{
			if(map[i]>=offset)
				break;
		}
		tempIterator.mapIter=map.begin()+i;
		tempIterator.vectorIter=data.begin()+i;
		tempIterator.listIter=data[i].begin();
	}else
	{
	    tempIterator.mapIter=map.end();
		tempIterator.vectorIter=data.end();
		tempIterator.listIter=tempIterator.vectorIter->end();
	}
	return(tempIterator);
}

template <typename ValueType>
inline typename HashMap<string, ValueType>::Iterator HashMap<string, ValueType>::begin(void)
{
	tempIterator.vectorPtr=&data;
	tempIterator.offset=offset;
	if(load)
	{
		int i=0;
		for(;i<capacity;++i)
		{
			if(map[i]>=offset)
				break;
		}
		tempIterator.mapIter=map.begin()+i;
		tempIterator.vectorIter=data.begin()+i;
		tempIterator.listIter=data[i].begin();
	}else
	{
	    tempIterator.mapIter=map.end();
		tempIterator.vectorIter=data.end();
		tempIterator.listIter=tempIterator.vectorIter->end();
	}
	return(tempIterator);
}

template <typename ValueType>
inline typename HashMap<char*, ValueType>::Iterator HashMap<char*, ValueType>::begin(void)
{
	tempIterator.vectorPtr=&data;
	tempIterator.offset=offset;
	if(load)
	{
		int i=0;
		for(;i<capacity;++i)
		{
			if(map[i]>=offset)
				break;
		}
		tempIterator.mapIter=map.begin()+i;
		tempIterator.vectorIter=data.begin()+i;
		tempIterator.listIter=data[i].begin();
	}else
	{
	    tempIterator.mapIter=map.end();
		tempIterator.vectorIter=data.end();
		tempIterator.listIter=tempIterator.vectorIter->end();
	}
	return(tempIterator);
}

template <typename ValueType>
inline typename HashMap<void*, ValueType>::Iterator HashMap<void*, ValueType>::begin(void)
{
	tempIterator.vectorPtr=&data;
	tempIterator.offset=offset;
	if(load)
	{
		int i=0;
		for(;i<capacity;++i)
		{
			if(map[i]>=offset)
				break;
		}
		tempIterator.mapIter=map.begin()+i;
		tempIterator.vectorIter=data.begin()+i;
		tempIterator.listIter=data[i].begin();
	}else
	{
	    tempIterator.mapIter=map.end();
		tempIterator.vectorIter=data.end();
		tempIterator.listIter=tempIterator.vectorIter->end();
	}
	return(tempIterator);
}

template <typename KeyType, typename ValueType>
inline typename HashMap<KeyType, ValueType>::Iterator HashMap<KeyType, ValueType>::back(void)
{
	tempIterator.vectorPtr=&data;
	tempIterator.offset=offset;
	if(load)
	{
		int i=capacity-1;
		for(;i>=0;--i)
		{
			if(map[i]>=offset)
				break;
		}
		tempIterator.mapIter=map.begin()+i;
		tempIterator.vectorIter=data.begin()+i;
		tempIterator.listIter=--data[i].end();
	}else
	{
	    tempIterator.mapIter=map.end();
		tempIterator.vectorIter=data.end();
		tempIterator.listIter=tempIterator.vectorIter->end();
	}
	return(tempIterator);
}

template <typename ValueType>
inline typename HashMap<string, ValueType>::Iterator HashMap<string, ValueType>::back(void)
{
	tempIterator.vectorPtr=&data;
	tempIterator.offset=offset;
	if(load)
	{
		int i=capacity-1;
		for(;i>=0;--i)
		{
			if(map[i]>=offset)
				break;
		}
		tempIterator.mapIter=map.begin()+i;
		tempIterator.vectorIter=data.begin()+i;
		tempIterator.listIter=--data[i].end();
	}else
	{
	    tempIterator.mapIter=map.end();
		tempIterator.vectorIter=data.end();
		tempIterator.listIter=tempIterator.vectorIter->end();
	}
	return(tempIterator);
}

template <typename ValueType>
inline typename HashMap<char*, ValueType>::Iterator HashMap<char*, ValueType>::back(void)
{
	tempIterator.vectorPtr=&data;
	tempIterator.offset=offset;
	if(load)
	{
		int i=capacity-1;
		for(;i>=0;--i)
		{
			if(map[i]>=offset)
				break;
		}
		tempIterator.mapIter=map.begin()+i;
		tempIterator.vectorIter=data.begin()+i;
		tempIterator.listIter=--data[i].end();
	}else
	{
	    tempIterator.mapIter=map.end();
		tempIterator.vectorIter=data.end();
		tempIterator.listIter=tempIterator.vectorIter->end();
	}
	return(tempIterator);
}

template <typename ValueType>
inline typename HashMap<void*, ValueType>::Iterator HashMap<void*, ValueType>::back(void)
{
	tempIterator.vectorPtr=&data;
	tempIterator.offset=offset;
	if(load)
	{
		int i=capacity-1;
		for(;i>=0;--i)
		{
			if(map[i]>=offset)
				break;
		}
		tempIterator.mapIter=map.begin()+i;
		tempIterator.vectorIter=data.begin()+i;
		tempIterator.listIter=--data[i].end();
	}else
	{
	    tempIterator.mapIter=map.end();
		tempIterator.vectorIter=data.end();
		tempIterator.listIter=tempIterator.vectorIter->end();
	}
	return(tempIterator);
}

template <typename KeyType, typename ValueType>
inline typename HashMap<KeyType, ValueType>::Iterator HashMap<KeyType, ValueType>::end(void)
{
    tempIterator.offset=offset;
	tempIterator.vectorPtr=&data;
	tempIterator.mapIter=map.end();
	tempIterator.vectorIter=data.end();
	tempIterator.listIter=tempIterator.vectorIter->end();
	return(tempIterator);
}

template <typename ValueType>
inline typename HashMap<string, ValueType>::Iterator HashMap<string, ValueType>::end(void)
{
    tempIterator.offset=offset;
    tempIterator.vectorPtr=&data;
	tempIterator.mapIter=map.end();
	tempIterator.vectorIter=data.end();
	tempIterator.listIter=tempIterator.vectorIter->end();
	return(tempIterator);
}

template <typename ValueType>
inline typename HashMap<char*, ValueType>::Iterator HashMap<char*, ValueType>::end(void)
{
    tempIterator.offset=offset;
    tempIterator.vectorPtr=&data;
	tempIterator.mapIter=map.end();
	tempIterator.vectorIter=data.end();
	tempIterator.listIter=tempIterator.vectorIter->end();
	return(tempIterator);
}

template <typename ValueType>
inline typename HashMap<void*, ValueType>::Iterator HashMap<void*, ValueType>::end(void)
{
    tempIterator.offset=offset;
    tempIterator.vectorPtr=&data;
	tempIterator.mapIter=map.end();
	tempIterator.vectorIter=data.end();
	tempIterator.listIter=tempIterator.vectorIter->end();
	return(tempIterator);
}

template <typename KeyType, typename ValueType>
ValueType HashMap<KeyType, ValueType>::remove(const Iterator& iter)
{
	--load;
	tempShkv.value=iter.listIter->value;
	tempShkv.hash=iter.listIter->hash;
	iter.vectorIter->erase(iter.listIter);
	if(iter.vectorIter->empty())
		map[tempShkv.hash & mask]=0;
	return(tempShkv.value);
}

template <typename ValueType>
ValueType HashMap<string, ValueType>::remove(const Iterator& iter)
{
	--load;
	tempShkv.value=iter.listIter->value;
	tempShkv.hash=iter.listIter->hash;
	iter.vectorIter->erase(iter.listIter);
	if(iter.vectorIter->empty())
		map[tempShkv.hash & mask]=0;
	return(tempShkv.value);
}

template <typename ValueType>
ValueType HashMap<char*, ValueType>::remove(const Iterator& iter)
{
	--load;
	tempShkv.value=iter.listIter->value;
	tempShkv.hash=iter.listIter->hash;
	iter.vectorIter->erase(iter.listIter);
	if(iter.vectorIter->empty())
		map[tempShkv.hash & mask]=0;
	return(tempShkv.value);
}

template <typename ValueType>
ValueType HashMap<void*, ValueType>::remove(const Iterator& iter)
{
	--load;
	tempShkv.value=iter.listIter->value;
	tempShkv.hash=iter.listIter->hash;
	iter.vectorIter->erase(iter.listIter);
	if(iter.vectorIter->empty())
		map[tempShkv.hash & mask]=0;
	return(tempShkv.value);
}

template <typename KeyType, typename ValueType>
bool HashMap<KeyType, ValueType>::containsKey(const KeyType& key)
{
	tempHash=superFastHash((const char*)&key, sizeof(key));
	if(map[tempHash & mask]>=offset)
	{
		for(dataIter=data[tempHash & mask].begin();
			dataIter!=data[tempHash & mask].end();
			++dataIter)
		{
			if(dataIter->key==key)
				return(true);
		}
	}
	return(false);
}

template <typename ValueType>
bool HashMap<string, ValueType>::containsKey(const string& key)
{
	tempHash=superFastHash(key.c_str(), key.size());
	if(map[tempHash & mask]>=offset)
	{
		for(dataIter=data[tempHash & mask].begin();
			dataIter!=data[tempHash & mask].end();
			++dataIter)
		{
			if(dataIter->key==key)
				return(true);
		}
	}
	return(false);
}

template <typename ValueType>
bool HashMap<char*, ValueType>::containsKey(const char* const key)
{
	tempHash=superFastHash(key, strlen(key));
	if(map[tempHash & mask]>=offset)
	{
		for(dataIter=data[tempHash & mask].begin();
			dataIter!=data[tempHash & mask].end();
			++dataIter)
		{
			if(!(strcmp(dataIter->key.c_str(), key)))
				return(true);
		}
	}
	return(false);
}

template <typename ValueType>
bool HashMap<void*, ValueType>::containsKey(const void* const key)
{
	tempHash=superFastHash((const char*)key, sizeof(void*));
	if(map[tempHash & mask]>=offset)
	{
		for(dataIter=data[tempHash & mask].begin();
			dataIter!=data[tempHash & mask].end();
			++dataIter)
		{
			if(dataIter->key==key)
				return(true);
		}
	}
	return(false);
}

template <typename KeyType, typename ValueType>
ValueType HashMap<KeyType, ValueType>::get(const KeyType& key)
{
	tempHash=superFastHash((const char*)&key, sizeof(key));
	if(map[tempHash & mask]>=offset)
	{
		for(dataIter=data[tempHash & mask].begin();
			dataIter!=data[tempHash & mask].end();
			++dataIter)
		{
			if(dataIter->key==key)
				return(dataIter->value);
		}
	}
	return(0);
}

template <typename ValueType>
ValueType HashMap<string, ValueType>::get(const string& key)
{
	tempHash=superFastHash(key.c_str(), key.size());
	if(map[tempHash & mask]>=offset)
	{
		for(dataIter=data[tempHash & mask].begin();
			dataIter!=data[tempHash & mask].end();
			++dataIter)
		{
			if(dataIter->key==key)
				return(dataIter->value);
		}
	}
	return(0);
}

template <typename ValueType>
ValueType HashMap<char*, ValueType>::get(const char* const key)
{
	tempHash=superFastHash(key, strlen(key));
	if(map[tempHash & mask]>=offset)
	{
		for(dataIter=data[tempHash & mask].begin();
			dataIter!=data[tempHash & mask].end();
			++dataIter)
		{
			if(!(strcmp(dataIter->key.c_str(), key)))
				return(dataIter->value);
		}
	}
	return(0);
}

template <typename ValueType>
ValueType HashMap<void*, ValueType>::get(const void* const key)
{
	tempHash=superFastHash((const char*)key, sizeof(void*));
	if(map[tempHash & mask]>=offset)
	{
		for(dataIter=data[tempHash & mask].begin();
			dataIter!=data[tempHash & mask].end();
			++dataIter)
		{
			if(dataIter->key==key)
				return(dataIter->value);
		}
	}
	return(0);
}

template <typename KeyType, typename ValueType>
typename HashMap<KeyType, ValueType>::Iterator HashMap<KeyType, ValueType>::getI(const KeyType& key)
{
	tempHash=superFastHash((const char*)&key, sizeof(key));
	if(map[tempHash & mask]>=offset)
	{
		for(dataIter=data[tempHash & mask].begin();
			dataIter!=data[tempHash & mask].end();
			++dataIter)
		{
			if(dataIter->key==key)
			{
				tempIterator.vectorPtr=&data;
				tempIterator.vectorIter=data.begin()+(tempHash & mask);
				tempIterator.listIter=dataIter;
				return(tempIterator);
			}
		}
	}
	return(end());
}

template <typename ValueType>
typename HashMap<string, ValueType>::Iterator HashMap<string, ValueType>::getI(const string& key)
{
	tempHash=superFastHash(key.c_str(), key.size());
	if(map[tempHash & mask]>=offset)
	{
		for(dataIter=data[tempHash & mask].begin();
			dataIter!=data[tempHash & mask].end();
			++dataIter)
		{
			if(dataIter->key==key)
			{
				tempIterator.vectorPtr=&data;
				tempIterator.vectorIter=data.begin()+(tempHash & mask);
				tempIterator.listIter=dataIter;
				return(tempIterator);
			}
		}
	}
	return(end());
}

template <typename ValueType>
typename HashMap<char*, ValueType>::Iterator HashMap<char*, ValueType>::getI(const char* const key)
{
	tempHash=superFastHash(key, strlen(key));
	if(map[tempHash & mask]>=offset)
	{
		for(dataIter=data[tempHash & mask].begin();
			dataIter!=data[tempHash & mask].end();
			++dataIter)
		{
			if(!(strcmp(dataIter->key.c_str(), key)))
			{
				tempIterator.vectorPtr=&data;
				tempIterator.vectorIter=data.begin()+(tempHash & mask);
				tempIterator.listIter=dataIter;
				return(tempIterator);
			}
		}
	}
	return(end());
}

template <typename ValueType>
typename HashMap<void*, ValueType>::Iterator HashMap<void*, ValueType>::getI(const void* const key)
{
	tempHash=superFastHash((const char*)key, sizeof(void*));
	if(map[tempHash & mask]>=offset)
	{
		for(dataIter=data[tempHash & mask].begin();
			dataIter!=data[tempHash & mask].end();
			++dataIter)
		{
			if(dataIter->key==key)
			{
				tempIterator.vectorPtr=&data;
				tempIterator.vectorIter=data.begin()+(tempHash & mask);
				tempIterator.listIter=dataIter;
				return(tempIterator);
			}
		}
	}
	return(end());
}

template <typename KeyType, typename ValueType>
ValueType HashMap<KeyType, ValueType>::put(KeyType& key, const ValueType& value)
{
	tempHash=superFastHash((const char*)&key, sizeof(key));
	if(map[tempHash & mask]>=offset)
	{
		for(dataIter=data[tempHash & mask].begin();
			dataIter!=data[tempHash & mask].end();
			++dataIter)
		{
			if(dataIter->key==key)
			{
				tempShkv.value=dataIter->value;
				dataIter->value=value;
				return(tempShkv.value);
			}
		}
		++load;
		tempShkv.hash=tempHash;
		tempShkv.key=key;
		tempShkv.value=value;
		data[tempHash & mask].push_front(tempShkv);
	}else
	{
		++load;
		map[tempHash & mask]=offset;
		tempShkv.hash=tempHash;
		tempShkv.key=key;
		tempShkv.value=value;
		data[tempHash & mask].clear();
		data[tempHash & mask].push_front(tempShkv);
	}
	if((float)(load/capacity)>highLoadFactor)
		increaseSize(1);
	return(0);
}

template <typename ValueType>
ValueType HashMap<string, ValueType>::put(string& key, const ValueType& value)
{
	tempHash=superFastHash(key.c_str(), key.size());
	if(map[tempHash & mask]>=offset)
	{
		for(dataIter=data[tempHash & mask].begin();
			dataIter!=data[tempHash & mask].end();
			++dataIter)
		{
			if(dataIter->key==key)
			{
				tempShkv.value=dataIter->value;
				dataIter->value=value;
				return(tempShkv.value);
			}
		}
		++load;
		tempShkv.hash=tempHash;
		tempShkv.key=key;
		tempShkv.value=value;
		data[tempHash & mask].push_front(tempShkv);
	}else
	{
		++load;
		map[tempHash & mask]=offset;
		tempShkv.hash=tempHash;
		tempShkv.key=key;
		tempShkv.value=value;
		data[tempHash & mask].clear();
		data[tempHash & mask].push_front(tempShkv);
	}
	if((float)(load/capacity)>highLoadFactor)
		increaseSize(1);
	return(0);
}

template <typename ValueType>
ValueType HashMap<char*, ValueType>::put(char* const key, const ValueType& value)
{
	tempHash=superFastHash(key, strlen(key));
	if(map[tempHash & mask]>=offset)
	{
		for(dataIter=data[tempHash & mask].begin();
			dataIter!=data[tempHash & mask].end();
			++dataIter)
		{
			if(!(strcmp(dataIter->key.c_str(), key)))
			{
				tempShkv.value=dataIter->value;
				dataIter->value=value;
				return(tempShkv.value);
			}
		}
		++load;
		tempShkv.hash=tempHash;
		tempShkv.key=key;
		tempShkv.value=value;
		data[tempHash & mask].push_front(tempShkv);
	}else
	{
		++load;
		map[tempHash & mask]=offset;
		tempShkv.hash=tempHash;
		tempShkv.key=key;
		tempShkv.value=value;
		data[tempHash & mask].clear();
		data[tempHash & mask].push_front(tempShkv);
	}
	if((float)(load/capacity)>highLoadFactor)
		increaseSize(1);
	return(0);
}

template <typename ValueType>
ValueType HashMap<void*, ValueType>::put(void* const key, const ValueType& value)
{
	tempHash=superFastHash((const char*)key, sizeof(void*));
	if(map[tempHash & mask]>=offset)
	{
		for(dataIter=data[tempHash & mask].begin();
			dataIter!=data[tempHash & mask].end();
			++dataIter)
		{
			if(dataIter->key==key)
			{
				tempShkv.value=dataIter->value;
				dataIter->value=value;
				return(tempShkv.value);
			}
		}
		++load;
		tempShkv.hash=tempHash;
		tempShkv.key=key;
		tempShkv.value=value;
		data[tempHash & mask].push_front(tempShkv);
	}else
	{
		++load;
		map[tempHash & mask]=offset;
		tempShkv.hash=tempHash;
		tempShkv.key=key;
		tempShkv.value=value;
		data[tempHash & mask].clear();
		data[tempHash & mask].push_front(tempShkv);
	}
	if((float)(load/capacity)>highLoadFactor)
		increaseSize(1);
	return(0);
}

template <typename KeyType, typename ValueType>
ValueType HashMap<KeyType, ValueType>::remove(const KeyType& key)
{
	tempHash=superFastHash((const char*)&key, sizeof(key));
	if(map[tempHash & mask]>=offset)
	{
		for(dataIter=data[tempHash & mask].begin();
			dataIter!=data[tempHash & mask].end();
			++dataIter)
		{
			if(dataIter->key==key)
			{
				--load;
				tempShkv.value=dataIter->value;
				data[tempHash & mask].erase(dataIter);
				if(data[tempHash & mask].empty())
					map[tempHash & mask]=0;
				if(((float)(load/capacity)<lowLoadFactor) && (capacity>16))
					decreaseSize(1);
				return(tempShkv.value);
			}
		}
	}
	return(0);
}

template <typename ValueType>
ValueType HashMap<string, ValueType>::remove(const string& key)
{
	tempHash=superFastHash(key.c_str(), key.size());
	if(map[tempHash & mask]>=offset)
	{
		for(dataIter=data[tempHash & mask].begin();
			dataIter!=data[tempHash & mask].end();
			++dataIter)
		{
			if(dataIter->key==key)
			{
				--load;
				tempShkv.value=dataIter->value;
				data[tempHash & mask].erase(dataIter);
				if(data[tempHash & mask].empty())
					map[tempHash & mask]=0;
				if(((float)(load/capacity)<lowLoadFactor) && (capacity>16))
					decreaseSize(1);
				return(tempShkv.value);
			}
		}
	}
	return(0);
}

template <typename ValueType>
ValueType HashMap<char*, ValueType>::remove(const char* const key)
{
	tempHash=superFastHash(key, strlen(key));
	if(map[tempHash & mask]>=offset)
	{
		for(dataIter=data[tempHash & mask].begin();
			dataIter!=data[tempHash & mask].end();
			++dataIter)
		{
			if(!(strcmp(dataIter->key.c_str(), key)))
			{
				--load;
				tempShkv.value=dataIter->value;
				data[tempHash & mask].erase(dataIter);
				if(data[tempHash & mask].empty())
					map[tempHash & mask]=0;
				if(((float)(load/capacity)<lowLoadFactor) && (capacity>16))
					decreaseSize(1);
				return(tempShkv.value);
			}
		}
	}
	return(0);
}

template <typename ValueType>
ValueType HashMap<void*, ValueType>::remove(const void* const key)
{
	tempHash=superFastHash((const char*)key, sizeof(key));
	if(map[tempHash & mask]>=offset)
	{
		for(dataIter=data[tempHash & mask].begin();
			dataIter!=data[tempHash & mask].end();
			++dataIter)
		{
			if(dataIter->key==key)
			{
				--load;
				tempShkv.value=dataIter->value;
				data[tempHash & mask].erase(dataIter);
				if(data[tempHash & mask].empty())
					map[tempHash & mask]=0;
				if(((float)(load/capacity)<lowLoadFactor) && (capacity>16))
					decreaseSize(1);
				return(tempShkv.value);
			}
		}
	}
	return(0);
}

template <typename KeyType, typename ValueType>
void HashMap<KeyType, ValueType>::increaseSize (const int powerOffset)
{
	power+=powerOffset;
    mask=0xffffffff>>(32-power);
	data.resize(0x1<<power);
	map.resize(0x1<<power, 0);

	for(int i=0;i<capacity;++i)
	{
		if(map[i]>=offset)
		{
			for(dataIter=data[i].begin();
				dataIter!=data[i].end();
				)
			{
				if((dataIter->hash & mask)!=(unsigned int)i)
				{
					map[dataIter->hash & mask]=offset;
					data[dataIter->hash & mask].push_front(*dataIter);
					tempdataIter=dataIter;
					++dataIter;
					data[i].erase(tempdataIter);
					continue;
				}
				++dataIter;
			}
			if(data[i].empty())
				map[i]=0;
		}
	}
	capacity=0x1<<power;
}

template <typename ValueType>
void HashMap<string, ValueType>::increaseSize (const int powerOffset)
{
	power+=powerOffset;
    mask=0xffffffff>>(32-power);
	data.resize(0x1<<power);
	map.resize(0x1<<power, 0);

	for(int i=0;i<capacity;++i)
	{
		if(map[i]>=offset)
		{
			for(dataIter=data[i].begin();
				dataIter!=data[i].end();
				)
			{
				if((dataIter->hash & mask)!=(unsigned int)i)
				{
					map[dataIter->hash & mask]=offset;
					data[dataIter->hash & mask].push_front(*dataIter);
					tempdataIter=dataIter;
					++dataIter;
					data[i].erase(tempdataIter);
					continue;
				}
				++dataIter;
			}
			if(data[i].empty())
				map[i]=0;
		}
	}
	capacity=0x1<<power;
}

template <typename ValueType>
void HashMap<char*, ValueType>::increaseSize (const int powerOffset)
{
	power+=powerOffset;
    mask=0xffffffff>>(32-power);
	data.resize(0x1<<power);
	map.resize(0x1<<power, 0);

	for(int i=0;i<capacity;++i)
	{
		if(map[i]>=offset)
		{
			for(dataIter=data[i].begin();
				dataIter!=data[i].end();
				)
			{
				if((dataIter->hash & mask)!=i)
				{
					map[dataIter->hash & mask]=offset;
					data[dataIter->hash & mask].push_front(*dataIter);
					tempdataIter=dataIter;
					++dataIter;
					data[i].erase(tempdataIter);
					continue;
				}
				++dataIter;
			}
			if(data[i].empty())
				map[i]=0;
		}
	}
	capacity=0x1<<power;
}

template <typename ValueType>
void HashMap<void*, ValueType>::increaseSize (const int powerOffset)
{
	power+=powerOffset;
    mask=0xffffffff>>(32-power);
	data.resize(0x1<<power);
	map.resize(0x1<<power, 0);

	for(int i=0;i<capacity;++i)
	{
		if(map[i]>=offset)
		{
			for(dataIter=data[i].begin();
				dataIter!=data[i].end();
				)
			{
				if((dataIter->hash & mask)!=i)
				{
					map[dataIter->hash & mask]=offset;
					data[dataIter->hash & mask].push_front(*dataIter);
					tempdataIter=dataIter;
					++dataIter;
					data[i].erase(tempdataIter);
					continue;
				}
				++dataIter;
			}
			if(data[i].empty())
				map[i]=0;
		}
	}
	capacity=0x1<<power;
}

template <typename KeyType, typename ValueType>
void HashMap<KeyType, ValueType>::decreaseSize (const int powerOffset)
{
    power-=powerOffset;
    mask>>=powerOffset;

    int i=capacity-1;
    for(capacity>>=powerOffset;i>=capacity;--i)
	{
		if(map[i]>=offset)
		{
			for(dataIter=data[i].begin();
				dataIter!=data[i].end();
				++dataIter)
			{
				if(map[dataIter->hash & mask]<offset)
				{
					map[dataIter->hash & mask]=offset;
					data[dataIter->hash & mask].clear();
				}
				data[dataIter->hash & mask].push_front(*dataIter);
			}
		}
	}
	data.resize(capacity);
	map.resize(capacity);
}

template <typename ValueType>
void HashMap<string, ValueType>::decreaseSize (const int powerOffset)
{
    power-=powerOffset;
    mask>>=powerOffset;

    int i=capacity-1;
    for(capacity>>=powerOffset;i>=capacity;--i)
	{
		if(map[i]>=offset)
		{
			for(dataIter=data[i].begin();
				dataIter!=data[i].end();
				++dataIter)
			{
				if(map[dataIter->hash & mask]<offset)
				{
					map[dataIter->hash & mask]=offset;
					data[dataIter->hash & mask].clear();
				}
				data[dataIter->hash & mask].push_front(*dataIter);
			}
		}
	}
	data.resize(capacity);
	map.resize(capacity);
}

template <typename ValueType>
void HashMap<char*, ValueType>::decreaseSize (const int powerOffset)
{
    power-=powerOffset;
    mask>>=powerOffset;

    int i=capacity-1;
    for(capacity>>=powerOffset;i>=capacity;--i)
	{
		if(map[i]>=offset)
		{
			for(dataIter=data[i].begin();
				dataIter!=data[i].end();
				++dataIter)
			{
				if(map[dataIter->hash & mask]<offset)
				{
					map[dataIter->hash & mask]=offset;
					data[dataIter->hash & mask].clear();
				}
				data[dataIter->hash & mask].push_front(*dataIter);
			}
		}
	}
	data.resize(capacity);
	map.resize(capacity);
}

template <typename ValueType>
void HashMap<void*, ValueType>::decreaseSize (const int powerOffset)
{
    power-=powerOffset;
    mask>>=powerOffset;

    int i=capacity-1;
    for(capacity>>=powerOffset;i>=capacity;--i)
	{
		if(map[i]>=offset)
		{
			for(dataIter=data[i].begin();
				dataIter!=data[i].end();
				++dataIter)
			{
				if(map[dataIter->hash & mask]<offset)
				{
					map[dataIter->hash & mask]=offset;
					data[dataIter->hash & mask].clear();
				}
				data[dataIter->hash & mask].push_front(*dataIter);
			}
		}
	}
	data.resize(capacity);
	map.resize(capacity);
}


/*
The superFastHash hash function bellow is authored by Paul Hsieh
(http://www.azillionmonkeys.com/qed/bio.html)

It's released under the Paul Hsieh derivative license
(http://www.azillionmonkeys.com/qed/weblicense.html)
*/
template <typename KeyType, typename ValueType>
unsigned int HashMap<KeyType, ValueType>::superFastHash (const char *data, int len)
{
	unsigned int hash=static_cast<unsigned int>(len), tmp;
	int rem;

	if((len<=0) || (data==NULL))
		return 0;

	rem=len & 3;
	len>>=2;

	/* Main loop */
	for(;len>0;len--)
	{
		hash+=(*((const unsigned short *) (data)));
		tmp=((*((const unsigned short *) (data+2)))<<11) ^ hash;
		hash=(hash<<16) ^ tmp;
		data+=2*sizeof(unsigned short);
		hash+=hash>>11;
	}

	/* Handle end cases */
	switch(rem)
	{
		case 3:
			hash+=(*((const unsigned short *) (data)));
			hash^=hash<<16;
			hash^=data[sizeof(unsigned short)]<<18;
			hash+=hash>>11;
		break;

		case 2:
			hash+=(*((const unsigned short *) (data)));
			hash^=hash<<11;
			hash+=hash>>17;
		break;

		case 1:
			hash+=*data;
			hash^=hash<<10;
			hash+=hash>>1;
	}

	/* Force "avalanching" of final 127 bits */
	hash^=hash<<3;
	hash+=hash>>5;
	hash^=hash<<2;
	hash+=hash>>15;
	hash^=hash<<10;

	return hash;
}

template <typename ValueType>
unsigned int HashMap<string, ValueType>::superFastHash (const char *data, int len)
{
	unsigned int hash=len, tmp;
	int rem;

	if((len<=0) || (data==NULL))
		return 0;

	rem=len & 3;
	len>>=2;

	/* Main loop */
	for(;len>0;len--)
	{
		hash+=(*((const unsigned short *) (data)));
		tmp=((*((const unsigned short *) (data+2)))<<11) ^ hash;
		hash=(hash<<16) ^ tmp;
		data+=2*sizeof(unsigned short);
		hash+=hash>>11;
	}

	/* Handle end cases */
	switch(rem)
	{
		case 3:
			hash+=(*((const unsigned short *) (data)));
			hash^=hash<<16;
			hash^=data[sizeof(unsigned short)]<<18;
			hash+=hash>>11;
		break;

		case 2:
			hash+=(*((const unsigned short *) (data)));
			hash^=hash<<11;
			hash+=hash>>17;
		break;

		case 1:
			hash+=*data;
			hash^=hash<<10;
			hash+=hash>>1;
	}

	/* Force "avalanching" of final 127 bits */
	hash^=hash<<3;
	hash+=hash>>5;
	hash^=hash<<2;
	hash+=hash>>15;
	hash^=hash<<10;

	return hash;
}

template <typename ValueType>
unsigned int HashMap<char*, ValueType>::superFastHash (const char *data, int len)
{
	unsigned int hash=len, tmp;
	int rem;

	if((len<=0) || (data==NULL))
		return 0;

	rem=len & 3;
	len>>=2;

	/* Main loop */
	for(;len>0;len--)
	{
		hash+=(*((const unsigned short *) (data)));
		tmp=((*((const unsigned short *) (data+2)))<<11) ^ hash;
		hash=(hash<<16) ^ tmp;
		data+=2*sizeof(unsigned short);
		hash+=hash>>11;
	}

	/* Handle end cases */
	switch(rem)
	{
		case 3:
			hash+=(*((const unsigned short *) (data)));
			hash^=hash<<16;
			hash^=data[sizeof(unsigned short)]<<18;
			hash+=hash>>11;
		break;

		case 2:
			hash+=(*((const unsigned short *) (data)));
			hash^=hash<<11;
			hash+=hash>>17;
		break;

		case 1:
			hash+=*data;
			hash^=hash<<10;
			hash+=hash>>1;
	}

	/* Force "avalanching" of final 127 bits */
	hash^=hash<<3;
	hash+=hash>>5;
	hash^=hash<<2;
	hash+=hash>>15;
	hash^=hash<<10;

	return hash;
}

template <typename ValueType>
unsigned int HashMap<void*, ValueType>::superFastHash (const char *data, int len)
{
	unsigned int hash=len, tmp;
	int rem;

	if((len<=0) || (data==NULL))
		return 0;

	rem=len & 3;
	len>>=2;

	/* Main loop */
	for(;len>0;len--)
	{
		hash+=(*((const unsigned short *) (data)));
		tmp=((*((const unsigned short *) (data+2)))<<11) ^ hash;
		hash=(hash<<16) ^ tmp;
		data+=2*sizeof(unsigned short);
		hash+=hash>>11;
	}

	/* Handle end cases */
	switch(rem)
	{
		case 3:
			hash+=(*((const unsigned short *) (data)));
			hash^=hash<<16;
			hash^=data[sizeof(unsigned short)]<<18;
			hash+=hash>>11;
		break;

		case 2:
			hash+=(*((const unsigned short *) (data)));
			hash^=hash<<11;
			hash+=hash>>17;
		break;

		case 1:
			hash+=*data;
			hash^=hash<<10;
			hash+=hash>>1;
	}

	/* Force "avalanching" of final 127 bits */
	hash^=hash<<3;
	hash+=hash>>5;
	hash^=hash<<2;
	hash+=hash>>15;
	hash^=hash<<10;

	return hash;
}
/*
The superFastHash hash function above is authored by Paul Hsieh
(http://www.azillionmonkeys.com/qed/bio.html)

It's released under the Paul Hsieh derivative license
(http://www.azillionmonkeys.com/qed/weblicense.html)
*/

//Iterator
template <typename KeyType, typename ValueType>
MyIterator<KeyType, ValueType>::MyIterator()
{

}

template <typename KeyType, typename ValueType>
inline bool MyIterator<KeyType, ValueType>::operator==(const MyIterator& op2) const
{
	return(listIter==op2.listIter);
}

template <typename KeyType, typename ValueType>
inline bool MyIterator<KeyType, ValueType>::operator!=(const MyIterator& op2) const
{
	return(listIter!=op2.listIter);
}

template <typename KeyType, typename ValueType>
inline ValueType& MyIterator<KeyType, ValueType>::operator*() const
{
	return(listIter->value);
}

template <typename KeyType, typename ValueType>
inline MyIterator<KeyType, ValueType>& MyIterator<KeyType, ValueType>::operator++()
{
	++listIter;
	if(listIter==vectorIter->end())
	{
	    ++mapIter;
		++vectorIter;
		while(vectorIter!=vectorPtr->end())
		{
			if(*mapIter>=offset)
			{
				listIter=vectorIter->begin();
				return(*this);
			}
			++mapIter;
			++vectorIter;
		}

		listIter=vectorIter->end();
	}
	return(*this);
}

template <typename KeyType, typename ValueType>
inline MyIterator<KeyType, ValueType> MyIterator<KeyType, ValueType>::operator++(int)
{
	MyIterator tempIterator=*this;
	++listIter;
	if(listIter==vectorIter->end())
	{
	    ++mapIter;
		++vectorIter;
		while(vectorIter!=vectorPtr->end())
		{
			if(*mapIter>=offset)
			{
				listIter=vectorIter->begin();
				return(tempIterator);
			}
			++mapIter;
			++vectorIter;
		}

		listIter=vectorIter->end();
	}
	return(tempIterator);
}

template <typename KeyType, typename ValueType>
inline MyIterator<KeyType, ValueType>& MyIterator<KeyType, ValueType>::operator--()
{
	if(listIter==vectorIter->begin())
	{
		while(vectorIter!=vectorPtr->begin())
		{
		    --mapIter;
			--vectorIter;
			if(*mapIter>=offset)
			{
				listIter=vectorIter->end();
				--listIter;
				return(*this);
			}
		}

		//vectorIter=vectorPtr->end();
		listIter=vectorIter->begin();
		return(*this);
	}
	--listIter;
	return(*this);
}

template <typename KeyType, typename ValueType>
inline MyIterator<KeyType, ValueType> MyIterator<KeyType, ValueType>::operator--(int)
{
	MyIterator tempIterator=*this;
	if(listIter==vectorIter->begin())
	{
		while(vectorIter!=vectorPtr->begin())
		{
		    --mapIter;
			--vectorIter;
			if(*mapIter>=offset)
			{
				listIter=vectorIter->end();
				--listIter;
				return(tempIterator);
			}
		}

		//vectorIter=vectorPtr->end();
		listIter=vectorIter->begin();
		return(tempIterator);
	}
	--listIter;
	return(tempIterator);
}

template <typename KeyType, typename ValueType>
inline KeyType MyIterator<KeyType, ValueType>::getKey() const
{
	return(listIter->key);
}
