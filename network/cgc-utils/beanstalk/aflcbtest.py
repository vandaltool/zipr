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

afl_input_queue_directory = '/techx_share/cqe_challenges_fuzz/'
testing_work_directory = "/tmp/testing_dir/"
results_directory = "/techx_share/aflcbtest/"
handler_id = 'aflcbtest'

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
    cb_for_coverage_path = "%s/poller/for-coverage/" % cb_working_path
    cb_afl_input_queue_path = "%s/%s_bin_%s_afl_queue" % (afl_input_queue_directory, cb_name, cb_name)

    """
    logging.critical("cb_path: %s" % cb_path)
    logging.critical("cb_working_path: %s" % cb_working_path)
    logging.critical("cb_for_coverage_path: %s" % cb_for_coverage_path)
    logging.critical("cb_afl_input_queue_path: %s" % cb_afl_input_queue_path)
    """

    if job_id != handler_id:
        logging.critical('cbtest handler given job of type %s' % job_id) 
        return

    if not os.path.exists(cb_path):
        logging.critical("cb_path (%s) does not exist." % cb_path)
        return

    if not os.path.exists(cb_afl_input_queue_path):
        logging.critical("cb_afl_input_queue_path (%s) does not exist." % cb_afl_input_queue_path)
        if not debug:
            result_event = {
                'id' : 'aflcbtest',
                'csid' : cbtest_id,
                'cb' : cb_name,
                'ip' : netifaces.ifaddresses('eth0')[2][0]['addr'],
                'stats' : "Missing AFL results.",
            }
            beanstalk.put_dict(result_event, beanstalk_connection)
        return

    # working directory
    start_dir = os.getcwd()
    shutil.copytree(cb_path, cb_working_path)

    # bring in the afl input queue
    shutil.copytree(cb_afl_input_queue_path, cb_for_coverage_path)
    for f in glob.glob(os.path.join(cb_for_coverage_path, "*")):
        # convert each afl input into a xml file.
        file_to_xml.file_to_xml_file(f, "%s.xml" % f, cb_name)
        # delete the raw file.
        os.remove(f)

    try:
        resource.setrlimit(resource.RLIMIT_CORE, (resource.RLIM_INFINITY, resource.RLIM_INFINITY))
    except Exception as e:
       logging.critical('cbtest.py: error setting core file limits: %s' % str(e))

    args = ['/techx_share/techx_umbrella/scripts/coverage_test.sh',
            cb_working_path,
            cb_name]
    child = Popen(args, stdout=PIPE, stderr=PIPE, stdin=None)
    (std_out, std_error) = child.communicate()

    if debug == True or debug == False:
        logging.critical('child: %d' % child.returncode)
        logging.critical('stderr: %s' % std_error)
        logging.critical('stdout: %s' % std_out)

    shutil.copy("%s/build/master.trace_comparison" % cb_working_path, 
                "%s/%s" % (results_directory, cb_name))
    shutil.rmtree(cb_working_path)
    shutil.rmtree(testing_work_directory)

    if debug == True:
        print std_out
    else:
        result_event = {
            'id' : 'aflcbtest',
            'csid' : cbtest_id,
            'cb' : cb_name,
            'ip' : netifaces.ifaddresses('eth0')[2][0]['addr'],
            'stats' : str(child.returncode),
        }
        beanstalk.put_dict(result_event, beanstalk_connection)
    # back to original directory

# handle cbtest event as lead
def handle_event(event, beanstalk_connection, debug=False):
    if event['id'] != 'aflcbtest':
        logging.critical('cbtest handler given event of type %s' % event['id'])
    print event
    with open("%s/aflcbest_stats.output" % results_directory, "a") as results_file:
        results_file.write("%s:%s\n" % (event['cb'],event['stats']))

if __name__ == '__main__':
    job = {
        'id' : 'aflcbtest',
        'cbtest_id' : 100,
#        'cb_name' : 'NRFIN_00006',
        'cb_name' : 'CROMU_00003',
        'cb_dir' : '/usr/share/cgc-sample-challenges/cqe-challenges/',
    }
    handle_job(job, None, True)
