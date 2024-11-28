gcc -shared -fPIC include/gcd_euclid.c -o libraries/libgcd_euclid.so
gcc -shared -fPIC include/gcd_naive.c -o libraries/libgcd_naive.so
gcc -shared -fPIC include/integral_trapezoid.c -o libraries/libintegral_trapezoid.so
gcc -shared -fPIC include/integral_rectangle.c -o libraries/libintegral_rectangle.so