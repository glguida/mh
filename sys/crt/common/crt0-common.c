int main(int argc, char *argv[]);

void ___start(void)
{
	int argc = 0;
	char *argv[] = { };

	main(argc, argv);
	/* XXX: DO EXIT! */
	while (1);
}
