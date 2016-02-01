#
# This program is free software; you can redistribute it and/or modify 
# it under the terms of the GNU General Public License as published by 
# the Free Software Foundation; either version 2 of the License, or 
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful, but 
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
# for more details.
# 
# You should have received a copy of the GNU General Public License along 
# with this program; if not, write to the Free Software Foundation, Inc., 
# 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#
#
# This file is part of XDD
#
# Description: XDD Contrib build rules
#

#
# Contributed 3rd-party distributions
#
PARAMIKO_DIST := $(CONTRIB_DIR)/paramiko-1.13.0.tar.gz
PYRO_DIST := $(CONTRIB_DIR)/Pyro4-4.25.tar.gz
SERPENT_DIST := $(CONTRIB_DIR)/serpent-1.5.tar.gz

#
# Build rules for contrib
#
contrib: contrib/site-packages paramiko pyro4 serpent

install_contrib: install_contrib_site-packages

clean_contrib: clean_contrib_site-packages
	$(info Cleaning the $(OS) CONTRIB files )

.PHONY: contrib install_contrib clean_contrib


#
# Build rules for site-packages
#
contrib/site-packages: 
	@$(MKDIR) contrib/site-packages

install_contrib_site-packages: install_ecdsa install_paramiko install_pycrypto install_pyro4 install_serpent

clean_contrib_site-packages: clean_ecdsa clean_paramiko clean_pycrypto clean_pyro4 clean_serpent
	@$(RM) -r contrib/site-packages

.PHONY: install_contrib_site-packages clean_contrib_site-packages

#
# Build optional ecdsa
#
contrib/ecdsa: $(ECDSA_DIST)
ifneq ($(ECDSA_DIST),)
	$(MKDIR) $@
	tar -C $@ -xvzf $< --strip-components 1
endif

contrib/site-packages/ecdsa: contrib/ecdsa
ifneq ($(ECDSA_DIST),)
	cd $< && $(PYTHON) setup.py build
	cd $< && $(PYTHON) setup.py install --install-lib ../site-packages
endif

ecdsa: contrib/site-packages/ecdsa
ifneq ($(ECDSA_DIST),)
endif

contrib/module.mkinstall_ecdsa: contrib/site-packages/ecdsa
ifneq ($(ECDSA_DIST),)
	$(INSTALL) -d $(PYTHON_SITE_DIR)
	$(CPR) $< $(PYTHON_SITE_DIR)
endif

clean_ecdsa:
ifneq ($(ECDSA_DIST),)
	@echo "Cleaning the ecdsa site package"
	@$(RM) -r contrib/site-packages/ecdsa
	@$(RM) -r contrib/ecdsa
	@$(RM) contrib/site-packages/ecdsa-0.11-*.egg-info
endif

.PHONY: ecdsa clean_ecdsa install_ecdsa

#
# Build paramiko
#
contrib/paramiko: $(PARAMIKO_DIST)
	$(MKDIR) $@
	tar -C $@ -xvzf $< --strip-components 1

contrib/site-packages/paramiko: contrib/paramiko contrib/paramiko-hostbased-auth.diff
	cd $< && $(PATCH) -p1 < ../paramiko-hostbased-auth.diff
	cd $< && $(PYTHON) setup.py build
	cd $< && $(PYTHON) setup.py install --install-lib ../site-packages --old-and-unmanageable

ifneq ($(ECDSA_DIST),)
paramiko: ecdsa 
endif
ifneq ($(PYCRYPTO_DIST),)
paramiko: pycrypto
endif
paramiko: contrib/site-packages/paramiko

install_paramiko: contrib/site-packages/paramiko
	$(INSTALL) -d $(PYTHON_SITE_DIR)
	$(CPR) $< $(PYTHON_SITE_DIR)

clean_paramiko:
	@echo "Cleaning the paramiko site package"
	@$(RM) -r contrib/site-packages/paramiko
	@$(RM) -r contrib/paramiko
	@$(RM) -r contrib/site-packages/paramiko-1.13.0-*.egg-info

.PHONY: paramiko clean_paramiko install_paramiko

#
# Build optional PyCrypto
#
ifeq (clang, $(CC))
PYCRYPTO_PYTHON := ARCHFLAGS=-Wno-error=unused-command-line-argument-hard-error-in-future && $(PYTHON)
else
PYCRYPTO_PYTHON := $(PYTHON)
endif

contrib/pycrypto: $(PYCRYPTO_DIST)
ifneq ($(PYCRYPTO_DIST),)
	$(MKDIR) $@
	tar -C $@ -xvzf $< --strip-components 1
endif

contrib/site-packages/Crypto: contrib/pycrypto
ifneq ($(PYCRYPTO_DIST),)
	cd $< && $(PYCRYPTO_PYTHON) setup.py build 
	cd $< && $(PYCRYPTO_PYTHON) setup.py install --install-lib ../site-packages
endif

pycrypto: contrib/site-packages/Crypto
ifneq ($(PYCRYPTO_DIST),)
endif

install_pycrypto: contrib/site-packages/Crypto
ifneq ($(PYCRYPTO_DIST),)
	$(INSTALL) -d $(PYTHON_SITE_DIR)
	$(CPR) $< $(PYTHON_SITE_DIR)
endif

clean_pycrypto:
ifneq ($(PYCRYPTO_DIST),)
	@echo "Cleaning the PyCrypto site package"
	@$(RM) -r contrib/site-packages/Crypto
	@$(RM) -r contrib/pycrypto
	@$(RM) contrib/site-packages/pycrypto-2.6.1-*.egg-info
endif

.PHONY: clean_pycrypto pycrypto install_pycrypto

#
# Build Pyro4
#
contrib/Pyro4: $(PYRO_DIST)
	$(MKDIR) $@
	tar -C $@ -xvzf $< --strip-components 1

contrib/site-packages/Pyro4: contrib/Pyro4
	cd $< && $(PYTHON) setup.py build
	cd $< && $(PYTHON) setup.py install --install-lib ../site-packages --old-and-unmanageable

pyro4: contrib/site-packages/serpent.py contrib/site-packages/Pyro4

install_pyro4: contrib/site-packages/Pyro4
	$(INSTALL) -d $(PYTHON_SITE_DIR)
	$(CPR) $< $(PYTHON_SITE_DIR)

clean_pyro4:
	@echo "Cleaning the Pyro4 site package"
	@$(RM) -r contrib/site-packages/Pyro4
	@$(RM) -r contrib/Pyro4
	@$(RM) -r contrib/site-packages/Pyro4-4.25-*.egg-info

.PHONY: pyro4 clean_pyro4 install_pyro4

#
# Build serpent
#
contrib/serpent: $(SERPENT_DIST)
	$(MKDIR) $@
	tar -C $@ -xvzf $< --strip-components 1

contrib/site-packages/serpent.py: contrib/site-packages contrib/serpent
	$(CP) contrib/serpent/serpent.py $@

contrib/site-packages/serpent.pyc: contrib/site-packages/serpent.py
	$(PYC) contrib/serpent
	$(CP) contrib/serpent/serpent.pyc $@

serpent: contrib/site-packages/serpent.pyc

install_serpent: contrib/site-packages/serpent.py contrib/site-packages/serpent.pyc
	$(INSTALL) -d $(PYTHON_SITE_DIR)
	$(INSTALL) -c -m 644 contrib/site-packages/serpent.py $(PYTHON_SITE_DIR)/serpent.py
	$(INSTALL) -c -m 644 contrib/site-packages/serpent.pyc $(PYTHON_SITE_DIR)/serpent.pyc

clean_serpent:
	@echo "Cleaning the serpent site package"
	@$(RM) -r contrib/site-packages/serpent.py
	@$(RM) -r contrib/site-packages/serpent.pyc

.PHONY: serpent clean_serpent install_serpent

#
#
#
# Build test support infrastructure
#
contrib/mpil: contrib/mpil.o
	$(MPICC) $^ -o $@

contrib/mpil.o: contrib/mpil.c
	$(MPICC) -c $^ -o $@
