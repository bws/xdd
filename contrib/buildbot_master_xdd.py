#!/usr/bin/python
#
# The buildbot settings for XDD.  We assume the following build slaves are 
# defined in the master.cfg:
#
# c['slaves'] = []
# c['slaves'].append(BuildSlave("pod9", "banana"))
# c['slaves'].append(BuildSlave("pod7", "banana"))
# c['slaves'].append(BuildSlave("pod10", "banana"))
# c['slaves'].append(BuildSlave("pod11", "banana"))
# c['slaves'].append(BuildSlave("spry02", "banana"))
# c['slaves'].append(BuildSlave("natureboy", "banana"))
#
# In order to enable these tests, add the 
# following lines to the bottom of the default master.cfg
#
####### Import the configuration to build/test XDD
# import buildbot_master_xdd
# reload(buildbot_master_xdd)
# from buildbot_master_xdd import *
# buildbot_master_xdd.loadConfig(config=c)
#
# To retrieve the latest version of this file, run the following command:
#
# git archive --format=tar --prefix=xdd/ --remote=/ccs/proj/csc040/var/git/xdd.git master |tar xf - --strip=2 xdd/contrib/buildbot_master_xdd.py
#
#

# This uses the BuildmasterConfig object referenced in master.cfg
def loadConfig(config):

    ####### CHANGESOURCES
    # the 'change_source' setting tells the buildmaster how it should find out
    # about source code changes.  Here we point to the buildbot clone of pyflakes.
    from buildbot.changes.gitpoller import GitPoller
    from buildbot.changes.filter import ChangeFilter
    config['change_source'].append( GitPoller( 
      repourl = 'git@github.com:ORNL/xdd.git',
      workdir='gitpoller-workdir-xdd-master', 
      pollinterval=120, 
      branch='master',
      project='xdd'))

    xdd_filter = ChangeFilter(
      project = 'xdd',
      branch = 'testing')

    ####### BUILDERS
    # The 'builders' list defines the Builders, which tell Buildbot how to perform a build:
    # what steps, and which slaves can execute them.  Note that any particular build will
    # only take place on one slave.
    from buildbot.process.factory import BuildFactory, GNUAutoconf
    from buildbot.steps.source import Git
    from buildbot.steps.shell import ShellCommand, Configure, Compile, Test

    xdd_factory = BuildFactory()

    # Check out the source
    xdd_factory.addStep(Git(repourl='git@github.com:ORNL/xdd.git', mode='copy', branch='master'))

    # Generate the test configuration
    xdd_factory.addStep(ShellCommand(command=['./contrib/buildbot_gen_test_config.sh'], name="configuring"))

    # Compile the code
    xdd_factory.addStep(Compile(description=["compiling"]))

    # Install the code
    xdd_factory.addStep(ShellCommand(command=['make', 'install'], name="make install"))

    # Perform make check
    xdd_factory.addStep(ShellCommand(command=['make', 'check'], name="make check", maxTime=600))

    # Perform make test
    xdd_factory.addStep(Test(description=["make test"], maxTime=600))

    # Perform cleanup
    xdd_factory.addStep(ShellCommand(command=['pkill', '-f', 'xdd', '||', 'echo ""'], name='process cleanup', maxTime=60))

    # Add the XDD Build factory to each of the available builders described in the master.cfg
    from buildbot.config import BuilderConfig
#    config['builders'].append(BuilderConfig(name="xdd-rhel5-x86_64", slavenames=["pod7"], factory=xdd_factory, env={"XDDTEST_TIMEOUT": "900"}, category='xdd'))
#    config['builders'].append(BuilderConfig(name="xdd-rhel6-x86_64", slavenames=["pod9"], factory=xdd_factory,  env={"XDDTEST_TIMEOUT": "900"},category='xdd'))
#    config['builders'].append(BuilderConfig(name="xdd-sles10-x86_64", slavenames=["pod10"], factory=xdd_factory, env={"XDDTEST_TIMEOUT": "900"}, category='xdd'))
    config['builders'].append(BuilderConfig(name="xdd-sles11-x86_64", slavenames=["pod11"], factory=xdd_factory, env={"XDDTEST_TIMEOUT": "900"}, category='xdd'))
    config['builders'].append(BuilderConfig(name="xdd-osx-10-8", slavenames=["natureboy"], factory=xdd_factory, env={"XDDTEST_TIMEOUT": "900"}, category='xdd'))
#    config['builders'].append(BuilderConfig(name="xdd-rhel6-ppc64", slavenames=["spry02"], factory=xdd_factory, env={"XDDTEST_TIMEOUT": "900"}, category='xdd'))

    ####### SCHEDULERS
    # Configure the Schedulers, which decide how to react to incoming changes.  In this
    # case, just kick off a 'runtests' build

    # Configure the nightly testing so that every test lives in the same buildset
    from buildbot.schedulers.basic import SingleBranchScheduler
    from buildbot.schedulers.timed import Periodic,Nightly
    build_nightly_xdd=Nightly(name="xdd-nightly1", 
                              branch = "master",
                              properties={'owner' : ['durmstrang-io@email.ornl.gov']}, 
                              builderNames=["xdd-sles11-x86_64", "xdd-osx-10-8"],
                              hour = 2,
                              minute = 3)
    config['schedulers'].append(build_nightly_xdd)

    # Configure each force build seperately so that they live in differing buildsets
    from buildbot.schedulers.forcesched import ForceScheduler
#    config['schedulers'].append(ForceScheduler(name="xdd-force1", builderNames=["xdd-rhel5-x86_64"]))
#    config['schedulers'].append(ForceScheduler(name="xdd-force2", builderNames=["xdd-rhel6-x86_64"]))
#    config['schedulers'].append(ForceScheduler(name="xdd-force3", builderNames=["xdd-sles10-x86_64"]))
    config['schedulers'].append(ForceScheduler(name="xdd-force4", builderNames=["xdd-sles11-x86_64"]))
    config['schedulers'].append(ForceScheduler(name="xdd-force6", builderNames=["xdd-osx-10-8"]))
#    config['schedulers'].append(ForceScheduler(name="xdd-force7", builderNames=["xdd-rhel6-ppc64"]))

    ####### STATUS TARGETS
    # 'status' is a list of Status Targets. The results of each build will be
    # pushed to these targets. buildbot/status/*.py has a variety to choose from,
    # including web pages, email senders, and IRC bots.
    from buildbot.status.mail import MailNotifier
    xddMN = MailNotifier(fromaddr="durmstrang-io@email.ornl.gov", 
                         extraRecipients=['durmstrang-io@email.ornl.gov'],
                         categories='xdd', 
                         buildSetSummary=True, 
                         messageFormatter=xddSummaryMail)
    config['status'].append(xddMN)


#
# Generate the BuildSetSummary mail format for XDD's nightly build
# and test information
#
from buildbot.status.builder import Results
from buildbot.status.results import FAILURE, SUCCESS, WARNINGS, Results
import urllib
def xddSummaryMail(mode, name, build, results, master_status):
    """Generate a buildbot mail message and return a tuple of the subject,
       message text, and mail type."""
    
    # Construct the mail subject
    subject = ""
    if results == SUCCESS:
        subject = "[Buildbot] SUCCESS -- XDD Acceptance Test -- SUCCESS"
    else:
        subject = "[Buildbot] FAILURE -- XDD Acceptance Test -- FAILURE"

    # Construct the mail body
    body = ""
    body += "Build Host: %s (%s)\n" % (build.getSlavename(), name)
    body += "Build Result: %s\n" % Results[results]
    body += "Build Status: %s\n" % master_status.getURLForThing(build)
    #body += "Build Logs available at: %s\n" % urllib.quote(master_status.getBuildbotURL(), '/:')
    #body += "Flagged Build: %s\n" % build.getSlavename()
    if results != SUCCESS:
        body += "Failed tests: %s\n" % build.getText()
    body += "--\n\n"

    return { 'subject' : subject, 'body' : body, 'type' : 'plain' }


