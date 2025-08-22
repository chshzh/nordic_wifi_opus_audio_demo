"""
Copyright (c) 2025 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""

#!/usr/bin/env python3
"""
PPK2 8-Channel Audio Latency Analysis Script

Analyzes PPK2 digital channel recordings (D0-D7) to calculate audio pipeline latency (T1-T8)
for the Nordic WiFi Opus Audio Demo. Supports both separate D0-D7 columns and single 'D0-D7' binary string.

Usage:
	python ppk_audio_latency_analysis.py -i audio_test.csv
	python ppk_audio_latency_analysis.py -i audio_test.csv -o results.md
	python ppk_audio_latency_analysis.py -i audio_test.csv --verbose

Author: Nordic Semiconductor, improved by GitHub Copilot
License: LicenseRef-Nordic-5-Clause
"""

import argparse
import csv
import sys
import statistics
from typing import List, Dict, Optional
from dataclasses import dataclass

TIMING_LABELS = [
	("T1", "Gateway", "P0.26"),
	("T2", "Gateway", "P0.25"),
	("T3", "Gateway", "P0.07"),
	("T4", "Gateway", "P0.28"),
	("T5", "Headset", "P0.26"),
	("T6", "Headset", "P0.25"),
	("T7", "Headset", "P0.07"),
	("T8", "Headset", "P0.28"),
]

@dataclass
class Frame:
	t: List[Optional[float]]  # T1-T8 timestamps
	def is_complete(self):
		return all(x is not None for x in self.t)
	def latency(self, i, j):
		return self.t[j] - self.t[i] if self.t[j] is not None and self.t[i] is not None else None

def parse_csv(filename, verbose=False):
	timestamps = []
	d = [[] for _ in range(8)]
	try:
		with open(filename, 'r', newline='') as f:
			reader = csv.DictReader(f)
			fields = reader.fieldnames
			if not fields or 'Timestamp(ms)' not in fields:
				raise ValueError("Missing 'Timestamp(ms)' column")
			has_separate = all(f"D{i}" in fields for i in range(8))
			has_binary = 'D0-D7' in fields
			if not (has_separate or has_binary):
				raise ValueError("CSV must have D0-D7 columns or a 'D0-D7' binary string column")
			for row in reader:
				try:
					t = float(row['Timestamp(ms)'])
					if has_separate:
						bits = [int(row[f"D{i}"]) for i in range(8)]
					else:
						binstr = row['D0-D7'].strip()
						if binstr.startswith('0b'): binstr = binstr[2:]
						binstr = binstr.zfill(8)
						bits = [int(b) for b in binstr]
					timestamps.append(t)
					for i in range(8):
						d[i].append(bits[i])
				except Exception as e:
					if verbose:
						print(f"Warning: Skipping row: {e}")
		if verbose:
			print(f"Parsed {len(timestamps)} samples.")
		return timestamps, d
	except Exception as e:
		print(f"Error reading CSV: {e}")
		sys.exit(1)

def detect_rising_edges(timestamps, channel):
	edges = []
	prev = 0
	for t, v in zip(timestamps, channel):
		if prev == 0 and v == 1:
			edges.append(t)
		prev = v
	return edges

def reconstruct_frames(edge_lists, max_frame_gap=200.0, verbose=False):
	# edge_lists: list of 8 lists, each with rising edge timestamps for T1-T8
	frames = []
	idx = [0]*8
	while True:
		# Find next available T1
		if idx[0] >= len(edge_lists[0]):
			break
		t_frame = [None]*8
		t_frame[0] = edge_lists[0][idx[0]]
		last_time = t_frame[0]
		valid = True
		for i in range(1,8):
			# Find the first edge after last_time (within max_frame_gap)
			while idx[i] < len(edge_lists[i]) and edge_lists[i][idx[i]] < last_time:
				idx[i] += 1
			if idx[i] < len(edge_lists[i]) and edge_lists[i][idx[i]] - last_time <= max_frame_gap:
				t_frame[i] = edge_lists[i][idx[i]]
				last_time = t_frame[i]
			else:
				valid = False
				break
		if valid:
			frames.append(Frame(t_frame))
			for i in range(8):
				idx[i] += 1
		else:
			idx[0] += 1  # skip this T1, try next
	if verbose:
		print(f"Reconstructed {len(frames)} frames.")
	return frames

def calc_stats(values):
	if not values:
		return {}
	return {
		'count': len(values),
		'mean': statistics.mean(values),
		'median': statistics.median(values),
		'min': min(values),
		'max': max(values),
		'std': statistics.stdev(values) if len(values)>1 else 0.0
	}

def analyze(frames):
	# Latency metrics: (name, i, j)
	metrics = [
		("Input Buffering (T2-T1)", 0, 1),
		("Encoding (T3-T2)", 1, 2),
		("Network (T5-T4)", 3, 4),
		("Decoding (T7-T6)", 5, 6),
		("Output Buffering (T8-T7)", 6, 7),
		("End-to-End (T8-T1)", 0, 7),
	]
	results = {}
	for name, i, j in metrics:
		vals = [f.latency(i,j) for f in frames if f.is_complete() and f.latency(i,j) is not None]
		results[name] = calc_stats(vals)
	return results

def print_report(frames, stats, verbose=False):
	# Output timing points table after latency table
	print("\n---\n")
	print("### Frame-by-Frame Timing Points Table (Unit: ms)")
	timing_header = ["Frame"] + [f"T{i+1}" for i in range(8)]
	print("| " + " | ".join(timing_header) + " |")
	print("|" + "|".join(["---"]*len(timing_header)) + "|")
	for i, f in enumerate(frames):
		row = [str(i+1)]
		row += [f"{t:.2f}" if t is not None else "-" for t in f.t]
		print("| " + " | ".join(row) + " |")
	print("\n---\n")
	# Markdown table output for all frames
	print("\n---\n")
	print("### WIFI Audio Latency Table(Unit: ms)")
	header = [
		"Frame",
		"T2-T1 (Input Buffer)",
		"T3-T2 (Encoding)",
		"T5-T4 (Network)",
		"T7-T6 (Decoding)",
		"T8-T7 (Output Buffer)",
		"T8-T1 (Total E2E)"
	]
	print("| " + " | ".join(header) + " |")
	print("|" + "|".join(["---"]*len(header)) + "|")
	latencies = [[] for _ in range(6)]
	for i, f in enumerate(frames):
		row = [str(i+1)]
		vals = [
			f.latency(0,1),
			f.latency(1,2),
			f.latency(3,4),
			f.latency(5,6),
			f.latency(6,7),
			f.latency(0,7),
		]
		for j, v in enumerate(vals):
			if v is not None:
				latencies[j].append(v)
		row += [f"{v:.2f}" if v is not None else "-" for v in vals]
		print("| " + " | ".join(row) + " |")
	# Add summary rows
	def fmt(v):
		return f"{v:.2f}" if v is not None else "-"
	labels = ["Max", "Min", "Average"]
	for label in labels:
		row = [f"**{label}**"]
		for j in range(6):
			vals = latencies[j]
			if vals:
				if label == "Average":
					val = sum(vals)/len(vals)
				elif label == "Min":
					val = min(vals)
				elif label == "Max":
					val = max(vals)
			else:
				val = None
			row.append(fmt(val))
		print("| " + " | ".join(row) + " |")
	print("\n---\n")

def save_markdown(frames, stats, filename):
	with open(filename, 'w') as f:
		f.write("# Nordic WiFi Opus Audio Latency Analysis Report\n\n")
		f.write(f"**Total frames:** {len(frames)}\n\n")
		f.write(f"**Complete frames (T1-T8):** {sum(1 for f in frames if f.is_complete())}\n\n")
		for k, v in stats.items():
			if v:
				f.write(f"## {k}\n")
				f.write(f"| Metric | Value (ms) |\n|---|---|\n")
				f.write(f"| Count | {v['count']} |\n")
				f.write(f"| Mean | {v['mean']:.3f} |\n")
				f.write(f"| Median | {v['median']:.3f} |\n")
				f.write(f"| Min | {v['min']:.3f} |\n")
				f.write(f"| Max | {v['max']:.3f} |\n")
				f.write(f"| Std | {v['std']:.3f} |\n\n")
			else:
				f.write(f"## {k}\nNo valid data.\n\n")
		f.write("\n## First 10 Frames (Timestamps in ms)\n")
		f.write("| Frame | " + " | ".join([x[0] for x in TIMING_LABELS]) + " |\n")
		f.write("|---|" + "|".join(["---"]*8) + "|\n")
		for i, fr in enumerate(frames[:10]):
			f.write(f"| {i+1} | " + " | ".join(f"{t:.3f}" if t is not None else "-" for t in fr.t) + " |\n")
	print(f"\nResults saved to {filename}")

def main():
	parser = argparse.ArgumentParser(description="Analyze PPK2 8-channel audio latency measurements (T1-T8)")
	parser.add_argument('-i', '--input', required=True, help='Input CSV file from PPK2')
	parser.add_argument('-o', '--output', help='Output markdown file for results')
	parser.add_argument('--verbose', action='store_true', help='Verbose/debug output')
	args = parser.parse_args()

	timestamps, d = parse_csv(args.input, args.verbose)
	edge_lists = [detect_rising_edges(timestamps, d[i]) for i in range(8)]
	if args.verbose:
		for i, edges in enumerate(edge_lists):
			print(f"D{i} ({TIMING_LABELS[i][0]}): {len(edges)} rising edges detected.")
	frames = reconstruct_frames(edge_lists, verbose=args.verbose)
	stats = analyze(frames)
	print_report(frames, stats, verbose=args.verbose)
	if args.output:
		save_markdown(frames, stats, args.output)

if __name__ == "__main__":
	main()
