#!/usr/bin/python
#
# The buildbot settings for XDD.  In order to enable these tests, add the 
# following lines to the bottom of the default master.cfg
#
####### Import the configuration to build/test XDD
#import buildbot_master_xdd
#reload(buildbot_master_xdd)
#from buildbot_master_xdd import *
#buildbot_master_xdd.loadConfig(config=c)



from buildbot.status.builder import Results
from buildbot.status.results import FAILURE, SUCCESS, WARNINGS, Results
import urllib
def xddMail(mode, name, build, results, master_status):
    """Generate a buildbot mail message and return a tuple of message text
        and type."""

    ss = build.getSourceStamp()  
    prev = build.getPreviousBuild()
    text = ""
    if results == FAILURE:
        if "change" in mode and prev and prev.getResults() != results or \
               "problem" in mode and prev and prev.getResults() != FAILURE:
            text += "The Buildbot has detected a new failure"
        else:
            text += "The Buildbot has detected a failed build"
    elif results == WARNINGS:
        text += "The Buildbot has detected a problem in the build"
    elif results == SUCCESS:
        if "change" in mode and prev and prev.getResults() != results:
            text += "The Buildbot has detected a restored build"
        else:
            text += "The Buildbot has detected a passing build"

    if ss and ss.project:
        project = ss.project
    else:
        project = master_status.getTitle()
    text += " on builder %s while building %s.\n" % (name, project)

    if master_status.getURLForThing(build):
        text += "Full details are available at: %s\n" % master_status.getURLForThing(build)

    if master_status.getBuildbotURL():
        text += "Buildbot URL: %s\n" % urllib.quote(master_status.getBuildbotURL(), '/:')

    text += "Buildslave for this Build: %s\n" % build.getSlavename()
    text += "Build Reason: %s\n" % build.getReason()
    source = ""
    if ss and ss.branch:
        source += "[branch %s] " % ss.branch
    if ss and ss.revision:
        source += str(ss.revision)
    else:
        source += "HEAD"
    if ss and ss.patch:
        source += " (plus patch)"

    text += "Build Source Stamp: %s\n" % source
    t = build.getText()
    if t:
        t = ": " + " ".join(t)
    else:
        t = ""

    if results == SUCCESS:
        text += "Build succeeded!\n"
    elif results == WARNINGS:
        text += "Build Had Warnings%s\n" % t
    else:
        text += "BUILD FAILED%s\n" % t
    text += "----------------------------------"
    return { 'body' : text, 'type' : 'plain' }


# This uses the BuildmasterConfig object referenced in master.cfg
def loadConfig(config):

    ####### CHANGESOURCES
    # the 'change_source' setting tells the buildmaster how it should find out
    # about source code changes.  Here we point to the buildbot clone of pyflakes.
    from buildbot.changes.gitpoller import GitPoller
    from buildbot.changes.filter import ChangeFilter
    config['change_source'] = GitPoller( 
      repourl = '/ccs/proj/csc040/var/git/xdd.git',
      workdir='gitpoller-workdir-xdd-master', 
      pollinterval=120, 
      branch='master',
      project='xdd')

    xdd_filter = ChangeFilter(
      project = 'xdd',
      branch = 'testing')

    ####### SCHEDULERS
    # Configure the Schedulers, which decide how to react to incoming changes.  In this
    # case, just kick off a 'runtests' build
    from buildbot.schedulers.basic import SingleBranchScheduler
    from buildbot.schedulers.timed import Periodic,Nightly
    build_nightly_xdd=Nightly(
      name="xdd-nightly-build-and-test", 
      branch = "master",
      properties={'owner' : ['durmstrang-io@email.ornl.gov']}, 
      builderNames=["xdd-rhel5-x86_64", "xdd-rhel6-x86_64", "xdd-sles11-x86_64", "xdd-sles10-x86_64"],
      hour = 2,
      minute = 3)

    config['schedulers'].append(build_nightly_xdd)

    from buildbot.schedulers.forcesched import ForceScheduler
    config['schedulers'].append(ForceScheduler(
                        name="xdd-force",
                        builderNames=["xdd-rhel5-x86_64", "xdd-rhel6-x86_64", "xdd-sles11-x86_64", "xdd-sles10-x86_64" ]))

    ####### BUILDERS
    # The 'builders' list defines the Builders, which tell Buildbot how to perform a build:
    # what steps, and which slaves can execute them.  Note that any particular build will
    # only take place on one slave.
    from buildbot.process.factory import BuildFactory, GNUAutoconf
    from buildbot.steps.source import Git
    from buildbot.steps.shell import ShellCommand, Configure, Compile

    xdd_factory = BuildFactory()
    # check out the source
    xdd_factory.addStep(Git(repourl='/ccs/proj/csc040/var/git/xdd.git', mode='copy', branch='master'))
    xdd_factory.addStep(Configure())
    xdd_factory.addStep(Compile(description=["compiling"]))
    #factory.addStep(Compile(command=["./build-with-ompi.sh", "make", "test"], description=["testing"]))
    xdd_factory.addStep(ShellCommand(command=['./contrib/nightly_build_and_test_pods.sh'], name="configuring"))
    #factory.addStep(ShellCommand(command=['sleep','60'], maxTime=20, haltOnFailure=False))
    xdd_factory.addStep(ShellCommand(command=['./tests/scripts/test_acceptance1.sh'], name="test_acceptance1.sh"))
    xdd_factory.addStep(ShellCommand(command=['./tests/scripts/test_acceptance2.sh'], name="test_acceptance2.sh"))
    xdd_factory.addStep(ShellCommand(command=['./tests/scripts/test_acceptance3.sh'], name="test_acceptance3.sh"))
    xdd_factory.addStep(ShellCommand(command=['./tests/scripts/test_acceptance4.sh'], name="test_acceptance4.sh"))
    xdd_factory.addStep(ShellCommand(command=['./tests/scripts/test_acceptance_gethostip1.sh'], name="test_acceptance_gethostip1.sh"))
    xdd_factory.addStep(ShellCommand(command=['./tests/scripts/test_acceptance_truncate1.sh'], name="test_acceptance_truncate1.sh"))

    # this test is long and hard to debug
    #factory.addStep(ShellCommand(command=['./tests/scripts/test_acceptance_hpcs14.sh']))
    # fails
    #factory.addStep(ShellCommand(command=['./tests/scripts/test_acceptance_multinic1.sh']))
    # don't bother with these until threading is fixed
    #factory.addStep(ShellCommand(command=['./tests/scripts/test_acceptance_xdd-analysis1.sh']))

    xdd_factory.addStep(ShellCommand(command=['bash', '-x','./tests/scripts/test_acceptance_xddcp1.sh'], description=["Multi Threaded Test 1"], maxTime=1200, name="test_acceptance_xddcp1.sh"))
    xdd_factory.addStep(ShellCommand(command=['bash', '-x','./tests/scripts/test_acceptance_xddcp2.sh'], description=["Multi Threaded Test 2"], maxTime=1200, name="test_acceptance_xddcp2.sh"))
    xdd_factory.addStep(ShellCommand(command=['bash', '-x','./tests/scripts/test_acceptance_xddcp3.sh'], description=["Multi Threaded Test 3"], maxTime=1200, name="test_acceptance_xddcp3.sh"))
    xdd_factory.addStep(ShellCommand(command=['bash', '-x','./tests/scripts/test_acceptance_xddcp4.sh'], description=["Multi Threaded Test 4"], maxTime=1200, name="test_acceptance_xddcp4.sh"))
    xdd_factory.addStep(ShellCommand(command=['bash', '-x','./tests/scripts/test_acceptance_xddcp5.sh'], description=["Multi Threaded Test 5"], maxTime=1200, name="test_acceptance_xddcp5.sh"))
    xdd_factory.addStep(ShellCommand(command=['bash', '-x','./tests/scripts/test_acceptance_multinic1.sh'], description=["Multi NIC Test"], maxTime=1200, name="test_acceptance_multinic1.sh"))
    xdd_factory.addStep(ShellCommand(command=['bash', '-x','./tests/scripts/test_acceptance_xddcp-r-a-n.sh'], maxTime=1200, name="test_acceptance_xddcp-r-a-n.sh"))
    xdd_factory.addStep(ShellCommand(command=['bash', '-x','./tests/scripts/test_acceptance_xddcp-F-a-n.sh'], maxTime=1200, name="test_acceptance_xddcp-F-a-n.sh"))
    xdd_factory.addStep(ShellCommand(command=['bash', '-x','./tests/scripts/test_acceptance_xddcp-multinic-a.sh'], maxTime=1200, name="test_acceptance_xddcp-multinic-a.sh"))
    xdd_factory.addStep(ShellCommand(command=['bash', '-x','./tests/scripts/test_acceptance_xddcp-multinic-r-a-n.sh'], maxTime=1200, name="test_acceptance_xddcp-multinic-r-a-n.sh"))
    xdd_factory.addStep(ShellCommand(command=['bash', '-x','./tests/scripts/test_acceptance_xddcp1_single_thread.sh'], description=["Single Threaded Test 1"], maxTime=1200, name="test_acceptance_xddcp1_single_thread.sh"))
    xdd_factory.addStep(ShellCommand(command=['bash', '-x','./tests/scripts/test_acceptance_xddcp2_single_thread.sh'], description=["Single Threaded Test 2"], maxTime=1200, name="test_acceptance_xddcp2_single_thread.sh"))
    xdd_factory.addStep(ShellCommand(command=['bash', '-x','./tests/scripts/test_acceptance_xddcp3_single_thread.sh'], description=["Single Threaded Test 3"], maxTime=1200, name="test_acceptance_xddcp3_single_thread.sh"))
    xdd_factory.addStep(ShellCommand(command=['bash', '-x','./tests/scripts/test_acceptance_xddcp4_single_thread.sh'], description=["Single Threaded Test 4"], maxTime=1200, name="test_acceptance_xddcp4_single_thread.sh"))

    #factory.addStep(ShellCommand(command=['./tests/scripts/test_acceptance_xddcp_analysis.sh'], maxTime=600))
    #factory.addStep(ShellCommand(command=['./tests/scripts/test_acceptance_xddcp_analysis_kernel.sh'], maxTime=600))

    from buildbot.config import BuilderConfig
    config['builders'].append(BuilderConfig(name="xdd-rhel5-x86_64", slavenames=["pod7"], factory=xdd_factory, env={"XDDTEST_TIMEOUT": "900"}, category='xdd'))
    config['builders'].append(BuilderConfig(name="xdd-rhel6-x86_64", slavenames=["pod9"], factory=xdd_factory,  env={"XDDTEST_TIMEOUT": "900"},category='xdd'))
    config['builders'].append(BuilderConfig(name="xdd-sles10-x86_64", slavenames=["pod10"], factory=xdd_factory, env={"XDDTEST_TIMEOUT": "900"}, category='xdd'))
    config['builders'].append(BuilderConfig(name="xdd-sles11-x86_64", slavenames=["pod11"], factory=xdd_factory, env={"XDDTEST_TIMEOUT": "900"}, category='xdd'))
    config['builders'].append(BuilderConfig(name="xdd-rhel6-ppc64", slavenames=["spry02"], factory=xdd_factory, env={"XDDTEST_TIMEOUT": "900"}, category='xdd'))

    ####### STATUS TARGETS
    # 'status' is a list of Status Targets. The results of each build will be
    # pushed to these targets. buildbot/status/*.py has a variety to choose from,
    # including web pages, email senders, and IRC bots.
    from buildbot.status.mail import MailNotifier
    config['status'].append(MailNotifier(fromaddr="xdd-testing@ornl.gov", extraRecipients=['durmstrang-io@email.ornl.gov'], categories='xdd', buildSetSummary=True, messageFormatter=xddMail))

