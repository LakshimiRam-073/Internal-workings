package main

import (
	"context"
	"fmt"
	"sync"
)

type Bit struct {
	nthByte           uint64
	bitNumber         uint8
	resolvedBitNumber uint64
}

type Bitmap struct {
	bitArray   []byte
	NumChannel chan Bit
}

func NewBitMap() *Bitmap {

	bitmap := &Bitmap{
		bitArray:   make([]byte, 4),
		NumChannel: make(chan Bit, 1),
	}

	copy(bitmap.bitArray, []byte{0})
	return bitmap
}

func (bitmap *Bitmap) getFirstUnsetBit(ctx context.Context) {

	totalBatches := 8
	lenBitmap := len(bitmap.bitArray)
	batchSize := (lenBitmap + totalBatches - 1) / totalBatches // ceil division
	cctx, cancel := context.WithCancel(ctx)
	defer cancel()

	var wg sync.WaitGroup

	for i := 0; i < totalBatches; i++ {
		start := i * batchSize
		end := start + batchSize
		if end > lenBitmap {
			end = lenBitmap
		}
		if start >= lenBitmap {
			break
		}

		wg.Add(1)
		go func(from, to int) {
			defer wg.Done()

			for byteIdx := from; byteIdx < to; byteIdx++ {
				select {
				case <-cctx.Done():
					return
				default:
				}

				byteVal := bitmap.bitArray[byteIdx]
				if byteVal == 0xFF {
					continue
				}
				for bit := 0; bit < 8; bit++ {
					isSet := (byteVal >> bit) & 1
					if isSet == 0 {
						select {
						case bitmap.NumChannel <- Bit{
							bitNumber:         uint8(bit),
							nthByte:           uint64(byteIdx),
							resolvedBitNumber: (uint64(byteIdx) * 8) + uint64(bit),
						}:
							cancel()
							return
						case <-cctx.Done():
							return
						}
					}
				}
			}
		}(start, end)
	}

	// Wait for all goroutines to finish before falling back
	wg.Wait()

	// If no goroutine found an unset bit, append a new byte
	select {
	case <-cctx.Done():
		// A goroutine already found a result, nothing to do
	default:
		bitmap.bitArray = append(bitmap.bitArray, byte(0))
		newByteIdx := uint64(len(bitmap.bitArray) - 1)
		bitmap.NumChannel <- Bit{
			bitNumber:         uint8(0),
			nthByte:           newByteIdx,
			resolvedBitNumber: newByteIdx * 8,
		}
	}
}

func (bitmap *Bitmap) SetBit(bit Bit) error {

	if bit.nthByte >= uint64(len(bitmap.bitArray)) {
		return fmt.Errorf("Len is too high, since the len of bitArray is %d", len(bitmap.bitArray))
	}

	bitmap.bitArray[bit.nthByte] |= (1 << bit.bitNumber)
	return nil
}

func (bitmap *Bitmap) UnSetBit(bit Bit) error {

	if bit.nthByte >= uint64(len(bitmap.bitArray)) {
		return fmt.Errorf("Len is too high, since the len of bitArray is %d", len(bitmap.bitArray))
	}

	bitmap.bitArray[bit.nthByte] &= ^(1 << bit.bitNumber)
	return nil
}
