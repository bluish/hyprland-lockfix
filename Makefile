lockfix.so: main.cpp
	$(CXX) -shared -fPIC -std=c++2b -O2 -fcf-protection=full $(shell pkg-config --cflags pixman-1 libdrm hyprland) -o $@ $<

clean:
	rm -f lockfix.so

.PHONY: clean
