package main

import "fmt"

func main() {
	var n int32
	if _, err := fmt.Scan(&n); err != nil {
		panic(err)
	}
	var s int32 = 7
	for i := int32(0); i < n; i++ {
		s = (s*31 + i) % 1000003
	}
	fmt.Println(s)
}
