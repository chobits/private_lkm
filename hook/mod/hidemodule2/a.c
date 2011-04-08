int main()
{
	asm volatile ("int $0x80"::"a"(337));
	return 0;
}
