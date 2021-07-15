void write_string(int colour, const char *string)
{
    volatile char *video = (volatile char*)0xB8000;
    while( *string != 0 )
    {
        *video++ = *string++;
        *video++ = colour;
    }
}

void _kernel_main() {
    write_string(4, "FACE THE SELF, MAKE THE EGOS");
    return;
}