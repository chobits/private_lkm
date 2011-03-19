int main()
{
	int ret,b;

	asm volatile ("int $0x80\n\r":"=a"(ret):"a"(337),"b"(111));
	return ret;

}
