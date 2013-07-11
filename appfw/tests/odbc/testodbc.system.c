int main(int argc, char **argv)
{
	char *commandInject = getenv("INJECT");

	if (commandInject)
		system(commandInject);
	else
		system(argv[1]);
}
