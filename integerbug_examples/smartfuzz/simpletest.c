//  ./simpletest.exe -2343434
//  

int main (int argc, char** argv)
{
	int i = atol(argv[1]);
	unsigned int j = 0;

	if (i < 10)
	{
		j = i;   // there should be a signedness error here  unsigned <-- signed
		if ( j > 50) 
		{
		 printf("Surprise! \n"); 
		 return 1;
		}
	}

return 0;

}
