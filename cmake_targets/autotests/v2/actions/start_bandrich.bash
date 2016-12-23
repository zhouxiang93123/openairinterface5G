#enable control+C reception (to be refined if it does not work)
stty isig intr ^C

cd /tmp/oai_test_setup/oai
source oaienv
cd cmake_targets/autotests/v2/actions
python start_bandrich.py
sudo wvdial -C wvdial.bandrich.conf
