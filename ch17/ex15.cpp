#include <iostream>
#include <vector>
#include <functional>
#include <utility>
#include <ctime>
#include <map>

namespace Exercises
{
	template<class T>
	class Hash
	{
	public:
		size_t operator()(const T& key) const;
	};

	template<class T>
	size_t Hash<T>::operator()(const T& key) const
	{
		size_t res = 0;
		size_t len = sizeof(T);
		const char* p = reinterpret_cast<const char*>(&key);
		while(len--) res = (res<<1) ^ *p++;
		return res;
	}

	// integer hash algorithm taken from http://burtleburtle.net/bob/hash/integer.html
	template<>
	size_t Hash<int>::operator()(const int& key) const
	{
		int a = key;
	    a = (a ^ 61) ^ (a >> 16);
	    a = a + (a << 3);
	    a = a ^ (a >> 4);
	    a = a * 0x27d4eb2d;
	    a = a ^ (a >> 15);
	    return a;
	}

	typedef char* Pchar;
	template<>
	size_t Hash<char*>::operator()(const Pchar& key) const
	{
		size_t res = 0;
		Pchar p = key;
		while(*p) res = (res<<1)^*p++;
		return res;
	}

	template<>
	size_t Hash<std::string>::operator()(const std::string& key) const
	{
		size_t res = 0;
		typedef std::string::const_iterator CI;
		CI p = key.begin();
		CI end = key.end();
		while(p!=end) res = (res<<1) ^ *p++;
		return res;
	}

	template<class Key, class T, class H = Hash<Key>,
		class EQ = std::equal_to<Key>, class A = allocator<pair<const Key, T> > >
	class hash_map
	{
	public:
		typedef Key key_type;
		typedef T mapped_type;
		typedef std::pair<const Key, T> value_type;
		typedef A allocator_type;
		typedef typename A::reference reference;
		typedef typename A::const_reference const_reference;

		typedef typename A::size_type size_type;
		typedef typename A::difference_type difference_type;

		typedef H Hasher;
		typedef EQ key_equal;

		class value_equal : public std::binary_function<value_type,value_type,bool>
		{
			friend class hash_map;
		protected:
			EQ eq;
			value_equal(EQ e) : eq(e) {}
		public:
			bool operator()(const value_type& x, const value_type& y) const
				{ return eq(x.first,y.first); }
		};

	private:
		// representation
		struct Entry
		{
			value_type p;
			bool erased;
			Entry* next;
			int pos;
			Entry(key_type k, mapped_type v, Entry* n, int ppos) : p(k,v), next(n), pos(ppos), erased(false) {}
			Entry& operator=(const Entry& x)
			{
				erased = x.erased; next = x.next; pos = x.pos; p.second = x.p.second;
				*(const_cast<Key*>(&p.first)) = x.p.first; return *this;
			}
		};
		std::vector<Entry,typename A::template rebind<Entry>::other> v;
		std::vector<Entry*,typename A::template rebind<Entry*>::other> b;

		const T default_value;
		Hasher hash;
		key_equal eq;
		float max_load;
		float grow;
		size_type no_of_erased;

	public:
		template<class Iter, class T>
		class hash_iterator
		{
			Iter i;
			Iter end;
		public:
			hash_iterator() {}
			hash_iterator(Iter ii, Iter ee) : i(ii), end(ee) {}
			T& operator*() const { return i->p; }
			T* operator->() const { return &(i->p); }
			hash_iterator& operator++() { ++i; while(i != end && i->erased) ++i; return *this; }
			hash_iterator operator++(int) { hash_iterator i = *this; ++*this; return i; }
			hash_iterator& operator--() { --i; while(i->erased) --i; return *this; }
			hash_iterator operator--(int) { hash_iterator i = *this; --*this; return i; }
			bool operator==(const hash_iterator& x) { return x.i==i; }
			bool operator!=(const hash_iterator& x) { return !(*this==x); }
			Entry& entry() { return *i; }
		};

		typedef hash_iterator<typename std::vector<Entry>::iterator, value_type> iterator;
		typedef hash_iterator<typename std::vector<Entry>::reverse_iterator, value_type> reverse_iterator;

		template<class Iter, class T>
		class const_hash_iterator
		{
			Iter i;
			Iter end;
		public:
			const_hash_iterator() {}
			const_hash_iterator(Iter ii, Iter ee) : i(ii), end(ee) {}
			const_hash_iterator(const iterator& ii) : i(ii.i), end(ii.end) {}
			const T& operator*() const { return i->p; }
			const T* const operator->() const { return &(i->p); }
			const_hash_iterator& operator++() { ++i; while(i != end && i->erased) ++i; return *this; }
			const_hash_iterator operator++(int) { hash_iterator i = *this; ++*this; return i; }
			const_hash_iterator& operator--() { --i; while(i->erased) --i; return *this; }
			const_hash_iterator operator--(int) { hash_iterator i = *this; --*this; return i; }
			bool operator==(const const_hash_iterator& x) { return x.i==i; }
			bool operator!=(const const_hash_iterator& x) { return !(*this==x); }
			Entry& entry() { return *i; }
		};

		typedef const_hash_iterator<typename std::vector<Entry>::const_iterator, value_type> const_iterator;
		typedef const_hash_iterator<typename std::vector<Entry>::const_reverse_iterator, value_type> const_reverse_iterator;

		hash_map(const T& dv = T(), size_type n = 101, const H& h = H(), const EQ& e = EQ()) :
		  default_value(dv), b(n), no_of_erased(0), hash(h), eq(e)
		{
			set_load();
			v.reserve(static_cast<unsigned int>(max_load*b.size()));
		}

		void set_load(float m = 0.7, float g = 1.6)
		{
			max_load = m;
			grow = g;
		}

		template<class In> hash_map(In first, In last,
			const T& dv = T(), size_type n = 101, const H& hf = H(), const EQ& eq = EQ());
		~hash_map() {}

		iterator begin() { std::vector<Entry>::iterator i = v.begin(); while(i!=v.end() && i->erased) ++i; return iterator(i,v.end()); }
		const_iterator begin() const { std::vector<Entry>::const_iterator i = v.begin(); while(i!=vne.d() && i->erased) ++i; return const_iterator(i,v.end()); }
		iterator end() { return iterator(v.end(),v.end()); }
		const_iterator end() const { return const_iterator(v.end(),v.end()); }
		reverse_iterator rbegin() { std::vector<Entry>::reverse_iterator i = v.rbegin(); while(i!=v.rend() && i->erased) ++i; return reverse_iterator(i,v.rend()); };
		const_reverse_iterator rbegin() const { std::vector<Entry>::reverse_iterator i = v.rbegin(); while(i!=v.rend() && i->erased) ++i; return reverse_iterator(i,v.rend()); };
		reverse_iterator rend() { return reverse_iterator(v.rend(),v.rend()); };
		const_reverse_iterator rend() const { return const_reverse_iterator(v.rend(),v.rend()); };

		// map functions
		iterator find(const key_type& k);
		const_iterator find(const key_type& k) const;
		size_type count(const key_type& k) const { return find(k) == end() ? 0 : 1; };
		iterator lower_bound(const key_type& k) { return find(k); }
		const_iterator lower_bound(const key_type& k) const { return find(k); }
		iterator upper_bound(const key_type& k) { iterator i = find(k); return i!=end() ? ++i : i; }
		const_iterator upper_bound(const key_type& k) const { const_iterator i = find(k); return i!=end() ? ++i : i; }
		std::pair<iterator,iterator> equal_range(const key_type& k) { iterator i,j; i=j=find(k); return std::make_pair(i,j!=end()?++j:j); }
		std::pair<const_iterator,const_iterator> equal_range(const key_type& k) const { const_iterator i,j; i=j=find(k); return std::make_pair(i,j!=end()?++j:j); }
		mapped_type& operator[](const key_type& k);

		// list functions
		std::pair<iterator, bool> insert(const value_type& val);
		iterator insert(iterator pos, const value_type& val) { return insert(val).first; }
		template<class In> void insert(In first, In last) { while(first != last) { insert(*first++); } }
		void erase(iterator pos) { if(pos.entry().erased==false) no_of_erased++; pos.entry().erased = true; }
		size_type erase(const key_type& k) { iterator i=find(k); if(i!=end()) { erase(i); return 1; } return 0; }
		void erase(iterator first, iterator last) { while(first!=last) erase(first++); }
		void clear() { std::fill(b.begin(),b.end(),static_cast<Entry*>(0)); v.clear(); no_of_erased = 0; }

		// other functions
		size_type size() const { return v.size()-no_of_erased; };
		size_type max_size() const { v.max_size(); };
		size_type bucket_count() const { return b.size(); }
		void resize(size_type n);
		Hasher hash_fun() const { return hash; }
		key_equal key_eq() const { return eq; }
		bool empty() const { return size()==0; }
		void swap(hash_map& x) { b.swap(x.b); v.swap(x.v); std::swap(no_of_erased,x.no_of_erased); }
		hash_map& operator=(const hash_map&);

		// added to test distribution
		float bucket_utilization() const { int count=0;for(size_type i=0;i<b.size();i++)if(b[i])count++; return (float)count/(float)b.size(); }
	};

	template<class Key, class T, class H, class EQ, class A>
	typename hash_map<Key,T,H,EQ,A>::iterator hash_map<Key,T,H,EQ,A>::find(const key_type& k)
	{
		size_type i = hash(k)%b.size();
		for(Entry* p = b[i]; p; p = p->next)
		{
			if(eq(k,p->p.first) && !p->erased)
			{
				return iterator(v.begin()+p->pos, v.end());
			}
		}
		return iterator(v.end(),v.end());
	}

	template<class Key, class T, class H, class EQ, class A>
	typename hash_map<Key,T,H,EQ,A>::const_iterator hash_map<Key,T,H,EQ,A>::find(const key_type& k) const
	{
		size_type i = hash(k)%b.size();
		for(Entry* p = b[i]; p; p = p->next)
		{
			if(eq(k,p->p.first) && !p->erased)
			{
				return const_iterator(v.begin()+p->pos, v.end());
			}
		}
		return const_iterator(v.end(),v.end());
	}

	template<class Key, class T, class H, class EQ, class A>
	void hash_map<Key,T,H,EQ,A>::resize(size_type s)
	{
		size_type i = v.size();
		if(s <= b.size()) return;
		while(no_of_erased)
		{
			if(v[--i].erased)
			{
				v.erase(v.begin()+i);
				--no_of_erased;
			}
		}

		b.resize(s);
		std::fill(b.begin(),b.end(),static_cast<Entry*>(0));
		v.reserve(static_cast<unsigned int>(s*max_load));
		for(size_type i = 0; i<v.size(); i++)
		{
			size_type ii = hash(v[i].p.first)%b.size();
			v[i].next = b[ii];
			v[i].pos = i;
			b[ii] = &v[i];
		}
	}

	template<class Key, class T, class H, class EQ, class A>
	typename hash_map<Key,T,H,EQ,A>::mapped_type& 
	hash_map<Key,T,H,EQ,A>::operator[](const key_type& k)
	{
		size_type i = hash(k)%b.size();
		for(Entry* p = b[i]; p; p = p->next)
		{
			if(eq(k,p->p.first))
			{
				if(p->erased)
				{
					p->erased = false;
					no_of_erased--;
					return p->p.second = default_value;
				}
				return p->p.second;
			}
		}
		if(size_type(b.size()*max_load) <= v.size())
		{
			resize(static_cast<unsigned int>(b.size()*grow));
			return operator[](k);
		}
		v.push_back(Entry(k,default_value,b[i],v.size()));
		b[i] = &v.back();
		return b[i]->p.second;
	}

	template<class Key, class T, class H, class EQ, class A>
	std::pair<typename hash_map<Key,T,H,EQ,A>::iterator, bool> hash_map<Key,T,H,EQ,A>::insert(const value_type& val)
	{
		iterator i = find(val.first);
		if(i!=end()) return std::make_pair(i, false);
		operator[](val.first) = val.second;
		return std::make_pair(find(val.first), true);
	}
}

struct BadHashType
{
	int k;
	int x[8];
	void init() {for(int i=0;i<8;i++) x[i]=0;}
	BadHashType() : k(0) { init(); }
	BadHashType(int kk) : k(kk) { init(); }
	bool operator==(const BadHashType& x) const { return k==x.k; }
};

int main()
{
	using namespace std;
	using namespace Exercises;

	typedef hash_map<int,int> IntInt;
	IntInt h;

	h[1] = 2;
	h[2] = 40;
	h[3] = 25;
	h[4] = 50;
	h[5] = 100;
	h[7] = 150;

	h.insert(IntInt::value_type(6, 125));

	vector<pair<int,int> > v;
	v.push_back(make_pair(10,1000));
	v.push_back(make_pair(15,1500));
	v.push_back(make_pair(20,2000));

	h.insert(v.begin(),v.end());

	IntInt::iterator i = h.find(4);
	h.erase(i);

	cout << "forward" << endl;
	for(IntInt::iterator i = h.begin(); i != h.end(); i++)
	{
		cout << i->first << '=' << i->second << endl;
	}

	h.erase(3);
	h[5] = 101;
	cout << "backward" << endl;
	for(IntInt::reverse_iterator i = h.rbegin(); i != h.rend(); i++)
	{
		cout << i->first << '=' << i->second << endl;
	}

	cout << h.count(0) << endl;
	cout << h.count(1) << endl;

	cout << h.lower_bound(1)->first << endl;
	cout << h.upper_bound(1)->first << endl;
	cout << (h.lower_bound(0) == h.end() && h.upper_bound(0) == h.end()) << endl;
	pair<IntInt::iterator,IntInt::iterator> pair = h.equal_range(1);
	cout << pair.first->first << pair.second->first << endl;

	h.clear();

	srand(static_cast<unsigned int>(time(0)));
	for(int i = 0; i < 10000; i++)
	{
		int r = rand();
		h[r] = r;
	}
	cout << "hash 1: bucket utilization: " << h.bucket_utilization() << endl;

	BadHashType bht;

	hash_map<BadHashType,int> bad;
	for(int i = 0; i < 10000; i++)
	{
		int r = rand();
		bad[BadHashType(r)] = r;
	}
	cout << "bad: bucket utilization: " << bad.bucket_utilization() << endl;
}
