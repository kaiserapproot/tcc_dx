// FEAT-6A-ext5: realistic use - a min-by-operator< over an array of objects,
// plus equality to confirm the found element.
class Item {
public:
    int key;
    int operator<(const Item& o) const { return key < o.key; }
    int operator==(const Item& o) const { return key == o.key; }
};

int main()
{
    Item xs[4];
    xs[0].key = 30;
    xs[1].key = 10;
    xs[2].key = 50;
    xs[3].key = 20;

    Item best = xs[0];
    int i;
    for (i = 1; i < 4; i = i + 1) {
        if (xs[i] < best)          // overloaded operator<
            best = xs[i];
    }

    Item target;
    target.key = 10;
    // best should be the key=10 item
    return (best == target) ? 0 : 1;
}
