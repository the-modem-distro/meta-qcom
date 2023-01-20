all: clean openqti

openqti:
	@${CC} ${LDFLAGS} -Wall -O2 src/dms_client.c src/wds_client.c src/space_mon.c src/thermal.c src/config.c src/scheduler.c src/pico2aud.c src/qmi.c src/timesync.c src/cell.c src/call.c src/command.c src/proxy.c src/sms.c src/tracking.c src/helpers.c src/atfwd.c src/logger.c src/md5sum.c src/ipc.c src/audio.c src/mixer.c src/pcm.c src/openqti.c -o openqti -lpthread -lttspico

	@chmod +x openqti

clean:
	@rm -rf openqti
