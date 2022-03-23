package main

import (
	"bufio"
	"fmt"
	"os"
	"os/exec"
)

type csource struct {
	filename string
	file     *os.File
	writer   *bufio.Writer
}

func createCSource(filename string) (s csource, err error) {
	if !flagQuiet {
		fmt.Fprintln(os.Stderr, "Writing:", filename)
	}

	fp, err := os.Create(filename)
	if err != nil {
		return s, err
	}
	return csource{
		filename: filename,
		file:     fp,
		writer:   bufio.NewWriter(fp),
	}, nil
}

func (s *csource) close() {
	if s.file != nil {
		s.file.Close()
		s.file = nil
	}
	if s.filename != "" {
		os.Remove(s.filename)
	}
}

func (s *csource) flush() error {
	if s.file == nil {
		panic("already closed")
	}
	err := s.writer.Flush()
	s.writer = nil
	if err != nil {
		return err
	}
	err = s.file.Close()
	s.file = nil
	if err != nil {
		return err
	}
	if flagFormat {
		cmd := exec.Command("clang-format", "-i", s.filename)
		if err := cmd.Run(); err != nil {
			return err
		}
	}
	s.filename = ""
	return nil
}
