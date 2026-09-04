struct Local {
    ~Local()
    {
    }
};

void use_local()
{
    static Local local_object;
}
