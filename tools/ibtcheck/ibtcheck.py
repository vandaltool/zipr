import sys
import argparse
from sets import Set

# only works for single-threaded programs

def convert_to_hex(x):
	converted = []
	for i in x:
		converted.append(hex(i))
	return converted

def display_violation(ib_src, ib_seen, allowed):
	print 'IBT violation detected at: ', hex(ib_src), '(src) --> ', hex(ib_seen), '(tgt) | allowed targets: ', convert_to_hex(allowed)

def display_violations(violations, ibtargets):
	for src in violations:
		for v in violations[src]:
			display_violation(src, v, ibtargets[src]['targets'])

		
if __name__ == "__main__":

	parser = argparse.ArgumentParser(description='Validate STARS IB COMPLETE annotations')
	parser.add_argument('stars_xrefs', type=file,
                   help='STARS Xrefs annotation file')
	parser.add_argument('trace_file', type=file,
                   help='QEMU-style instructions')

	args = parser.parse_args();

# stash away the annotations
#  cols   0         1  2     3    4       5       6        7
#       465d0f      1 INSTR XREF IBT    FROMIB   465c72 RETURNTARGET
#       465de1      1 INSTR XREF IBT    FROMIB   465c72 RETURNTARGET
#       465c72      1 INSTR XREF FROMIB COMPLETE    2
	ibtargets = {}
	for line in args.stars_xrefs:
		cols = line.split()
		if (len(cols) < 5):
			continue
		if cols[5] == 'FROMIB':
			src = long(cols[6], 16)
			dst = long(cols[0], 16)

			if not src in ibtargets:
				ibtargets[src] = {}

			if not 'targets' in ibtargets[src]:
				ibtargets[src]['targets'] = []

			if not 'complete' in ibtargets[src]:
				ibtargets[src]['complete'] = False

			ibtargets[src]['targets'].append(dst)
		elif cols[5] == 'COMPLETE':
			src = long(cols[0], 16)

			if not src in ibtargets:
				ibtargets[src] = {}

			ibtargets[src]['complete'] = True

			
# 0x000000000045d7b9:  retq   
# 0x00000040013575e0:  push   %r13

	violations = {}

	line_no = -1
	to_check = -1L
	for line in args.trace_file:
		line_no+=1

		if not line.startswith('0x'):
			continue

		try:
			cols = line.split(':')
			if len(cols) > 0:
				# handle 2 formats:
				# 0x004000f0: push %r11
				# 0x004000f0 <address> <some_function_here+xxx>: push %r11
				instr = cols[0].split(' ')[0]
				instr = long(instr, 16)
			else:
				instr = long(cols[0], 16)
		except:
			print 'warning: parse error on line: ', line_no, ' ', line

		if to_check >= 0:
			if not instr in ibtargets[to_check]['targets']:
				print 'Detected ibtarget violation at ', hex(to_check), ' --> ', hex(instr), '(', len(ibtargets[to_check]['targets']), ')', convert_to_hex(ibtargets[to_check]['targets'])	
				if not to_check in violations:
					violations[to_check] = Set()
				violations[to_check].add(instr)
			else:
				if not 'covered' in ibtargets[to_check]:
					ibtargets[to_check]['covered'] = Set()

				ibtargets[to_check]['covered'].add(instr)
					
			to_check = -1L

		# did STARS mark complete?
		# if yes check the next instruction
		if instr in ibtargets and ibtargets[instr]['complete'] == True:
			to_check = instr		
	
	# get some statistics
	total_ibtargets = 0
	ibtargets_covered = 0
	icfs_coverage_count = 0
	for src_instr in ibtargets:
		total_ibtargets += len(ibtargets[src_instr]['targets'])
		if 'covered' in ibtargets[src_instr]:
			icfs_coverage_count += 1
			ibtargets_covered += len(ibtargets[src_instr]['covered'])

	if len(violations) > 0:
		print '==========================================='
		display_violations(violations, ibtargets)
		print '==========================================='
	else:
		print 'No ICFS violations detected'

	print '#icfs_covered: ', icfs_coverage_count, '/', len(ibtargets), ' ratio: ', icfs_coverage_count * 1.0 / len(ibtargets)

	print '#ibtargets_covered: ', ibtargets_covered, '/', total_ibtargets, ' ratio: ', 1.0*ibtargets_covered / total_ibtargets

	if len(violations) > 0:
		exit(1)
	else:
		exit(0)

