template <typename T>
struct A
{
    T a;
    A(T c) : a(c) {}
    void show(T p);
};

template <typename T>
void A<T>::show(T p)
{
    printf("%d, %d\n", p, a);
}