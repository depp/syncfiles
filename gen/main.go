package main

import (
	"flag"
	"fmt"
	"os"
	"path/filepath"
)

var (
	flagDumpSequences   bool
	flagDumpTransitions bool
)

func init() {
	flag.BoolVar(&flagDumpSequences, "dump-sequences", false, "dump Unicode sequences")
	flag.BoolVar(&flagDumpTransitions, "dump-transitions", false, "dump state machine state transition tables")
}

func getSrcdir() (string, error) {
	exe, err := os.Executable()
	if err != nil {
		return "", err
	}
	return filepath.Dir(filepath.Dir(exe)), nil
}

func mainE() error {
	srcdir, err := getSrcdir()
	if err != nil {
		return fmt.Errorf("could not find source dir: %v", err)
	}
	if err := os.Chdir(srcdir); err != nil {
		return err
	}
	d, err := readData()
	if err != nil {
		return err
	}
	_ = d
	return nil
}

func main() {
	flag.Parse()
	if err := mainE(); err != nil {
		fmt.Fprintln(os.Stderr, "Error:", err)
		os.Exit(1)
	}
}
