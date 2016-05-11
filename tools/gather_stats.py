# gather_stats.py <file1> <file2> ... <fileN>

import os
import sys
import re
import json

ATTRIBUTE='# ATTRIBUTE'
ATTRIBUTE2='#ATTRIBUTE'

# extract attribute key and value from attribute line
# format:    # ATTRIBUTE key=value 
# format:    #ATTRIBUTE key=value
def extract_attribute(attribute_line):
	line=''
	if ATTRIBUTE in attribute_line:
		line = attribute_line[len(ATTRIBUTE):].lstrip()
	elif ATTRIBUTE2 in attribute_line:
		line = attribute_line[len(ATTRIBUTE2):].lstrip()
	equal_pos = line.find('=')
	return (line[0:equal_pos].rstrip(), line[equal_pos+1:].lstrip())

# get stats one file at a time
stats = {}
for log_file_path in sys.argv[1:]:
	ps_analyze_log = False
	log_file = os.path.basename(log_file_path)

	if not os.path.exists(log_file_path):
		continue

	if log_file == 'ps_analyze.log':
		ps_analyze_log = True

	step_name = os.path.splitext(log_file)[0]

	with open(log_file_path, 'r') as f:
		for line in f:
			line = line.rstrip()
			if not line or line is None:
				continue
		
			if ps_analyze_log:
				if 'ATTRIBUTE start_time' in line or 'ATTRIBUTE end_time' in line or 'ATTRIBUTE hostname' in line:
					(attribute_name, attribute_value) = extract_attribute(line.rstrip())
					if not step_name in stats:
						stats[step_name] = {}
					stats[step_name][attribute_name] = attribute_value
			elif re.search(ATTRIBUTE, line) or re.search(ATTRIBUTE2, line): 
				(attribute_name, attribute_value) = extract_attribute(line.rstrip())
				if not step_name in stats:
					stats[step_name] = {}
				stats[step_name][attribute_name] = attribute_value

if stats:
	print json.dumps(stats)
	
