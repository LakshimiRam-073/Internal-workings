package main

import (
	"fmt"
	"unsafe"
)

type Key int
type Value int

type Pair struct {
	Key         Key
	Value       Value
	used        bool
	isProbed    bool
	probedIndex int
}

type HashTable struct {
	data     unsafe.Pointer
	capacity int
}

func (h *HashTable) moveByx(ptr unsafe.Pointer, x int) *Pair {
	size := unsafe.Sizeof(Pair{})
	return (*Pair)(unsafe.Pointer(uintptr(ptr) + uintptr(x)*size))
}

func (h *HashTable) getPair(ptr unsafe.Pointer) *Pair {
	return (*Pair)(ptr)
}
func (h *HashTable) hash(key Key) int {
	idx:= int(key) % h.capacity
	fmt.Printf("Hash(%d) = %d\n", key, idx)
	return idx
}

func (h *HashTable) Put(key Key, value Value) error {

	hashIndex := h.hash(key)
	selectedPair := h.moveByx(h.data, hashIndex)

	if !selectedPair.used {
		selectedPair.Key, selectedPair.Value = key, value
		selectedPair.used, selectedPair.isProbed = true, false
		selectedPair.probedIndex = -1
		return nil
	}
	if selectedPair.Key == key{
		selectedPair.Value = value
		return nil
	}
	for i := hashIndex + 1; i < h.capacity; i++ {
		selectedPair = h.moveByx(h.data, i)
		if !selectedPair.used {
			selectedPair.Key, selectedPair.Value = key, value
			selectedPair.used, selectedPair.isProbed = true, true
			selectedPair.probedIndex = (i-hashIndex)
			return nil
		}else if selectedPair.Key == key {
			selectedPair.Value=value
			return nil
		}
	}

	return fmt.Errorf("Capacity full can't linear probe through")
}

func (h *HashTable) Get(key Key) Value {

hashIndex := h.hash(key)

	for i := 0; i < h.capacity; i++ {
		idx := (hashIndex + i) % h.capacity
		pair := h.moveByx(h.data, idx)

		// If we hit an empty slot, key doesn't exist
		if !pair.used {
			return Value(-1)
		}

		// If key matches, return value
		if pair.Key == key {
			return pair.Value
		}
	}

	return Value(-1)

}

func (h *HashTable) Print() {
	size := unsafe.Sizeof(Pair{})

	for i := 0; i < h.capacity; i++ {
		ptr := unsafe.Pointer(uintptr(h.data) + uintptr(i)*size)
		pair := (*Pair)(ptr)

		if pair.used {
			fmt.Printf("[%d] Key=%d Value=%d (probed=%v, offset=%d)\n",
				i, pair.Key, pair.Value, pair.isProbed, pair.probedIndex)
		} else {
			fmt.Printf("[%d] EMPTY\n", i)
		}
	}
}
func NewHashMap(capacity int)(*HashTable){
	arr:= make([]Pair,capacity)
	
	return &HashTable{
		data: unsafe.Pointer(&arr[0]),
		capacity: capacity,
	}
}

func main() {

	hashMap:= NewHashMap(10)
	
	hashMap.Put(Key(12),Value(12))
	hashMap.Put(Key(22),Value(14))

	hashMap.Print()
	fmt.Print(hashMap.Get(Key(22)))
}
