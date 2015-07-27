# gather_stats.py <file1> <file2> ... <fileN>

import os
import sys
import re
import json

ATTRIBUTE='# ATTRIBUTE'

# extract attribute key and value from attribute line
# format:    # ATTRIBUTE key=value
def extract_attribute(attribute_line):
	line = attribute_line[len(ATTRIBUTE):].lstrip()
	equal_pos = line.find('=')
	return (line[0:equal_pos], line[equal_pos+1:])

# get stats one file at a time
for log_file_path in sys.argv[1:]:
	log_file = os.path.basename(log_file_path)

	if not os.path.exists(log_file_path):
		continue

	if log_file == 'ps_analyze.log':
		continue

	step_name= os.path.splitext(log_file)[0]

	stats = {}
	log_file_handle = open(log_file)
	for line in log_file_handle:
		line = line.rstrip()
		if not line or line is None:
			continue
		
		if re.search(ATTRIBUTE, line): 
			(attribute_name, attribute_value) = extract_attribute(line.rstrip())
			if not step_name in stats:
				stats[step_name] = {}

			stats[step_name][attribute_name] = attribute_value
	if stats:
		print json.dumps(stats)
	
