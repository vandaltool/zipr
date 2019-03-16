
FROM ubuntu:18.04
RUN DEBIAN_FRONTEND=noninteractive apt-get update &&   \
    DEBIAN_FRONTEND=noninteractive apt-get install -yq \
	postgresql  				\
	postgresql-client  			\
	libpqxx-dev				\
	sudo 					\
	nasm

ENV IRDB_LIBS=/opt/ps_zipr/irdb-libs/lib
COPY ./installed /opt/ps_zipr/
COPY ./run_pszr.sh /run_pszr.sh
RUN sudo chmod +x /run_pszr.sh
RUN cd /opt/ps_zipr && ./get-peasoup-packages.sh all
RUN useradd -ms /bin/bash zuser && gpasswd -a zuser sudo 
RUN echo '%sudo   ALL=(ALL:ALL) NOPASSWD:ALL' >> /etc/sudoers
RUN echo 'export PATH=$PATH:/opt/ps_zipr/tools/' >> /home/zuser/.bashrc 
RUN echo 'sudo service postgresql start ' >> /home/zuser/.bashrc
RUN chown zuser:zuser /home/zuser/.bashrc
USER zuser
ENV USER=zuser
RUN sudo service postgresql start && cd /opt/ps_zipr/ && (env USER=zuser ./postgres_setup.sh || true)
WORKDIR /home/zuser
ENTRYPOINT [ "/bin/bash", "-c", "/run_pszr.sh \"$@\"", "--" ]
CMD [ ] 
