// FEAT-4E-P2 x FEAT-4G: global object whose dtor body is out-of-class
// (checks the .fini_array thunk links against __cpp_dtor_G)
static int side;

class G {
public:
    int v;
    G() { v = 3; }
    ~G();
};

G::~G() { side = 1; }

G g;

int main()
{
    return g.v - 3;
}
