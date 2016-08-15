#!/usr/bin/python
# vim:ts=4:expandtab

import os
import subprocess
from subprocess import Popen, PIPE
import yaml
import glob
import shutil
import sys
import resource
import netifaces
import logging
import shlex

import file_to_xml

# our stuff
import config
import util
import beanstalk
import submission

testing_work_directory = "/tmp/testing_dir/"
results_directory = "/techx_share/cbtest/"
handler_id = 'cbtest'
coverage_input_path = "/techx_share/cqe_challenges_fuzz/"

# handle peasoupify job as worker
def handle_job(job, beanstalk_connection, debug=False):
    '''
    ps_job = {
        'id' : 'cbtest',
        'cbtest_id' : csid,
        'cb_name' : cb,
        'cb_dir' : working dir,
        'ps_extra_options' : '--step p1transform=off',    (optional)
    }
    '''

    job_id = job['id']
    cbtest_id = job['cbtest_id']
    cb_name = job['cb_name']
    cb_dir = job['cb_dir']
    ps_extra_options = job['ps_extra_options'] if 'ps_extra_options' in job else None

    cb_path = "%s/%s" % (cb_dir, cb_name)
    cb_working_path = "%s/%s" % (testing_work_directory, cbtest_id)
    # Format for coverage input path should be CBNAME_bin_CBNAME_afl_queue
    cb_coverage_input_path = "%s/%s_bin_%s_afl_queue/" % (coverage_input_path, cb_name, cb_name)

    if job_id != handler_id:
        logging.critical('cbtest handler given job of type %s' % job_id) 
        return

    if not os.path.exists(cb_path):
        logging.critical("cb_path (%s) does not exist." % cb_path)
        return

    # working directory
    start_dir = os.getcwd()
    shutil.copytree(cb_path, cb_working_path)

    if os.path.exists(cb_coverage_input_path):
        cb_for_coverage_path = "%s/poller/for-coverage/" % cb_working_path
        shutil.copytree(cb_coverage_input_path, cb_for_coverage_path)
        for f in glob.glob(os.path.join(cb_for_coverage_path,"*")):
            # convert each afl input into a xml file.
            file_to_xml.file_to_xml_file(f, "%s.xml" % f, cb_name)
            # delete the raw file.
            os.remove(f)
    elif 'require_coverage' in job and job['require_coverage']:
        result_event = {
            'id' : 'cbtest',
            'csid' : cbtest_id,
            'cb' : cb_name,
            'ip' : netifaces.ifaddresses('eth0')[2][0]['addr'],
            'stats' : "%s: N/A" % cb_name,
        }
        if debug == True:
            print(result_event)
        else:
            beanstalk.put_dict(result_event, beanstalk_connection)
        return

    try:
        resource.setrlimit(resource.RLIMIT_CORE, (resource.RLIM_INFINITY, resource.RLIM_INFINITY))
    except Exception as e:
       logging.critical('cbtest.py: error setting core file limits: %s' % str(e))

    args = ['/home/vagrant/cgc-utils/coverage_stats_test.sh',cb_working_path]+\
            shlex.split(ps_extra_options)
    child = Popen(args, stdout=PIPE, stderr=PIPE, stdin=None)
    (std_out, std_error) = child.communicate()

    if debug == True or debug == False:
        logging.critical('child: %d' % child.returncode)
        logging.critical('stderr: %s' % std_error)
        logging.critical('stdout: %s' % std_out)

    shutil.rmtree(cb_working_path)
    shutil.rmtree(testing_work_directory)
    if debug == True:
        print std_out
    else:
        result_event = {
            'id' : 'cbtest',
            'csid' : cbtest_id,
            'cb' : cb_name,
            'ip' : netifaces.ifaddresses('eth0')[2][0]['addr'],
            'stats' : std_out,
        }
        beanstalk.put_dict(result_event, beanstalk_connection)
    # back to original directory

# handle cbtest event as lead
def handle_event(event, beanstalk_connection, debug=False):
    if event['id'] != 'cbtest':
        logging.critical('cbtest handler given event of type %s' % event['id'])
    print event
    with open("%s/cgc_stats.output" % results_directory, "a") as results_file:
        results_file.write(event['stats'])

if __name__ == '__main__':
    job = {
        'id' : 'cbtest',
        'cbtest_id' : 100,
        'cb_name' : 'CROMU_00002',
        'cb_dir' : '/usr/share/cgc-sample-challenges/cqe-challenges/',
        'ps_extra_options': '--step-option zipr:\\\"--locality:on true\\\"',
        'require_coverage': True,
    }
    handle_job(job, None, True)
