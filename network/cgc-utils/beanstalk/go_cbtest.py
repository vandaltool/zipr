#!/usr/bin/env python

import sys
import os
#import argparse
import beanstalk

cb_dir="/usr/share/cgc-sample-challenges/cqe-challenges/"

if __name__ == "__main__":
    b = beanstalk.connect_as_lead()
    cbtest_id = 1
    ps_extra_options = '--step-option zipr:\\\"--locality:on true\\\"'
    for cb_name in os.listdir(cb_dir):
        j = {
             'id' : 'cbtest',
              'cbtest_id' : cbtest_id,
              'cb_name' : cb_name,
              'cb_dir' : cb_dir,
              'ps_extra_options': ps_extra_options,
	}
	print 'Submitting job: %s' % str(j)
	beanstalk.put_dict_priority(j, 0, b)
    #submit_cbtest_job(b)
