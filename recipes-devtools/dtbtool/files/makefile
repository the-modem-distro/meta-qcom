# This relies on ${LIBS} for an appropriate path to libmincrypt.a. The path in
# the Android tree is
# out/host/linux-x86/obj/STATIC_LIBRARIES/libmincrypt_intermediates/libmincrypt.a
dtbtool: dtbtool.o
	$(CXX) $(LDFLAGS) $< ${LIBS} -o $@

dtbtool.o: dtbtool.c
	$(CC) $(INCLUDES) $(CFLAGS) -c $<
