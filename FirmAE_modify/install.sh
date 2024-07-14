#!/bin/bash

# postgresql
sudo DEBIAN_FRONTEND=noninteractive apt install -y postgresql
sudo /etc/init.d/postgresql restart
sudo -u postgres bash -c "psql -c \"CREATE USER firmadyne WITH PASSWORD 'firmadyne';\""
sudo -u postgres createdb -O firmadyne firmware
sudo -u postgres psql -d firmware < ../FirmAE_modify/data
echo "listen_addresses = '172.17.0.1,127.0.0.1,localhost'" | sudo -u postgres tee --append /etc/postgresql/*/main/postgresql.conf
echo "host all all 172.17.0.1/24 trust" | sudo -u postgres tee --append /etc/postgresql/*/main/pg_hba.conf

sudo cp core/unstuff /usr/local/bin/

# for analyzer, initializer
cd ./analyses/routersploit && patch -p1 < ../routersploit_patch && cd -
