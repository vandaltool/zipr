// from smartfuzz paper
// ./simpletest.exe -2147483659 will trigger the Surprise
int main (int argc, char** argv)
{
	int i = atol(argv[1]); //[i is signed]
	unsigned int j = 0;

	if (i < 10) //[i is signed]
	{
		j = i;   // catch here? [unsigned = signed]
		if ( j > 50) //[j is unsigned]
		{
		 printf("Surprise! \n"); 
		 return 1;
		}
	}

return 0;

}
