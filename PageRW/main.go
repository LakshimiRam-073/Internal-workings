package main

import (
	"fmt"
	"os"
	"time"
)

const (
	FileName = "demo.db"
)

func benchmarkStorage(name string, storage PageStorage, numPages int) {
	d := NewDiskManagerWithStorage(storage)
	defer d.Close()

	data := EncodeData([]byte("rjodfijasdf90asdj0vasdofljlkames5rma0qwwemsdfasdft34"))

	// --- Write benchmark ---
	writeStart := time.Now()
	pages := make([]*Page, 0, numPages)
	for i := 0; i < numPages; i++ {
		page := d.NewPage(data)
		_, err := d.WritePage(page)
		if err != nil {
			fmt.Printf("[%s] Write error at page %d: %v\n", name, i, err)
			return
		}
		pages = append(pages, page)
	}
	err := d.Sync()
	if err != nil {
		fmt.Printf("[%s] Sync error: %v\n", name, err)
		return
	}
	writeDuration := time.Since(writeStart)

	// --- Read benchmark ---
	readStart := time.Now()
	for _, page := range pages {
		_, err := d.ReadPage(page.PageId)
		if err != nil {
			fmt.Printf("[%s] Read error at page %d: %v\n", name, page.PageId, err)
			return
		}
	}
	readDuration := time.Since(readStart)

	fmt.Printf("\n=== %s (%d pages) ===\n", name, numPages)
	fmt.Printf("  Write: %v  (%.2f pages/sec)\n", writeDuration, float64(numPages)/writeDuration.Seconds())
	fmt.Printf("  Read:  %v  (%.2f pages/sec)\n", readDuration, float64(numPages)/readDuration.Seconds())
}

func main() {
	for _, numPages := range []int{100, 1000, 10000} {
		// --- FileStorage benchmark ---
		fileDB := fmt.Sprintf("bench_file_%d.db", numPages)
		fs, err := NewFileStorage(fileDB)
		if err != nil {
			fmt.Println(err)
			return
		}
		benchmarkStorage("FileStorage", fs, numPages)
		os.Remove(fileDB)

		// --- MmapStorage benchmark ---
		mmapDB := fmt.Sprintf("bench_mmap_%d.db", numPages)
		ms, err := NewMmapStorage(mmapDB)
		if err != nil {
			fmt.Println(err)
			return
		}
		benchmarkStorage("MmapStorage", ms, numPages)
		os.Remove(mmapDB)
	}
}
