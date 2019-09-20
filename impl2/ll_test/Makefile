DEPS = ll_hwinit_display.o ll_delay.o ll_out_display.o

compile: $(DEPS)
	
%.o: %.c ../interface/%.h
	$(CC) $(CFLAGS) -c $*.c

clean:
	rm $(DEPS) > /dev/null 2>&1 || true
