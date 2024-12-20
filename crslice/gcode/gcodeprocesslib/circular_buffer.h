#pragma once
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gcode Processor Library
//
// Tools for parsing gcode and calculating printer state from parsed gcode commands.

//
// Copyright(C) 2021 - Brad Hochgesang
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This program is free software : you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU Affero General Public License for more details.
//
//
// You can contact the author at the following email address: 
// FormerLurker@pm.me
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <exception>
template <typename T>
class circular_buffer
{
public:
	circular_buffer()
	{
		max_size_ = 50;
		front_index_ = 0;
		count_ = 0;
		items_ = new T[max_size_];
	}

	circular_buffer(int max_size)
	{
		max_size_ = max_size;
		front_index_ = 0;
		count_ = 0;
		items_ = new T[max_size];
	}

	virtual ~circular_buffer() {
		delete[] items_;
	}

	void initialize(T object)
	{
		for (int index = 0; index < max_size_; index++)
		{
			push_back(object);
		}
		count_ = 0;
		front_index_ = 0;
	}

	void resize(int max_size)
	{
		T* new_items = new T[max_size];
		for (int index = 0; index < count_; index++)
		{
			new_items[index] = items_[(front_index_ + index + max_size_) % max_size_];
		}
		front_index_ = 0;
		delete[] items_;
		items_ = new_items;
		max_size_ = max_size;
	}

	void resize(int max_size, T object)
	{
		T* new_items = new T[max_size];
		for (int index = 0; index < count_; index++)
		{
			new_items[index] = items_[(front_index_ + index + max_size_) % max_size_];
		}
		// Initialize the rest of the entries
		for (int index = count_; index < max_size; index++)
		{
			new_items[index] = object;
		}
		front_index_ = 0;
		delete[] items_;
		items_ = new_items;
		max_size_ = max_size;
	}

	inline int get_index_position(int index) const
	{
		int index_position = index + front_index_ + max_size_;
		while (index_position >= max_size_)
		{
			index_position = index_position - max_size_;
		}
		return index_position;
	}

	void push_front(T object)
	{
		//front_index_ = (front_index_ - 1 + max_size_) % max_size_;
		front_index_ -= 1;
		if (front_index_ < 0)
		{
			front_index_ = max_size_ - 1;
		}
		if (count_ != max_size_)
		{
			count_++;
		}
		items_[front_index_] = object;
	}

	void push_back(T object)
	{
		int pos = get_index_position(count_);
		items_[pos] = object;
		count_++;
		if (count_ != max_size_)
		{
			count_++;
		}
	}

	T& pop_front()
	{
		if (count_ == 0)
		{
			throw std::exception();
		}

		int prev_start = front_index_;

		front_index_ += 1;
		if (front_index_ >= max_size_)
		{
			front_index_ = 0;
		}
		count_--;
		return items_[prev_start];
	}

	T& pop_back()
	{
		if (count_ == 0)
		{
			throw std::exception();
		}
		int pos = get_index_position(count_ - 1);
		count_--;
		return items_[pos];
	}

	T& operator[] (int index) const
	{
		//int opos = get_index_position(index);
		return items_[get_index_position(index)];
	}

	T& get(int index) const
	{
		int opos = get_index_position(index);
		return items_[opos];
	}

	int count() const
	{
		return count_;
	}

	int get_max_size() const
	{
		return max_size_;
	}

	void clear()
	{
		count_ = 0;
		front_index_ = 0;
	}

	void copy(const circular_buffer<T>& source)
	{
		if (max_size_ < source.max_size_)
		{
			resize(source.max_size_);
		}
		clear();
		for (int index = 0; index < source.count_; index++)
		{
			items_[index] = source[index];
		}
		front_index_ = source.front_index_;
		count_ = source.count_;
	}

protected:
	T* items_;
	int  max_size_;
	int  front_index_;
	int  count_;
};