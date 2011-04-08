int main()
{
	int ret;
	asm volatile ("int $0x80":"=a"(ret):"a"(337));
	return ret;
}
