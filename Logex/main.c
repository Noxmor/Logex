#include "clap/clap.h"

#include <math.h>

typedef struct LogexTree
{
	char left_var;
	char right_var;

	uint8_t left_negated;
	uint8_t right_negated;

	char operand;

	struct LogexTree* left_child;
	struct LogexTree* right_child;

	struct LogexTree* parent;
} LogexTree;

typedef struct
{
	uint8_t values[26];
} Occupancy;

void printLogexTree(LogexTree* root)
{
	if (root->left_child)
	{
		printf("(");
		printLogexTree(root->left_child);
		printf(")");
	}
	else
	{
		if (root->left_negated)
			printf("!");
	
		printf("%c %c ", root->left_var, root->operand);
	}

	if (root->right_child)
	{
		printf("(");
		printLogexTree(root->right_child);
		printf(")");
	}
	else
	{
		if (root->right_negated)
			printf("!");
	
		printf("%c", root->right_var);
	}

	if (root->parent == NULL)
		printf("\n");
}

uint8_t evaluateLogexTree(LogexTree* root, Occupancy* occupancy)
{
	uint8_t left_result = 0;

	if (root->left_child)
		left_result = evaluateLogexTree(root->left_child, occupancy);
	else
		left_result = occupancy->values[root->left_var - 'A'];

	if (root->left_negated)
		left_result = !left_result;

	uint8_t right_result = 0;
	if (root->right_child)
		right_result = evaluateLogexTree(root->right_child, occupancy);
	else
		right_result = occupancy->values[root->right_var - 'A'];

	if (root->right_negated)
		right_result = !right_result;

	switch (root->operand)
	{
		case '&': return left_result && right_result;
		case '|': return left_result || right_result;
	}

	return 0;
}

LogexTree* logexToLogexTree(const char* logex)
{
	LogexTree* root = calloc(1, sizeof(root));
	root->parent = NULL;
	root->left_child = NULL;
	root->right_child = NULL;

	LogexTree* current = root;

	size_t len = strlen(logex);

	uint8_t next_is_negated = 0;

	for (size_t i = 0; i < len; i++)
	{
		char c = logex[i];

		switch(c)
		{
			case ' ': continue;

			case '(':
			{
				if (current->left_child == NULL && current->left_var == 0)
				{
					current->left_child = calloc(1, sizeof(LogexTree));
					current->left_child->parent = current;
					current->left_negated = next_is_negated;
					current = current->left_child;
				}
				else
				{
					current->right_child = calloc(1, sizeof(LogexTree));
					current->right_child->parent = current;
					current->right_negated = next_is_negated;
					current = current->right_child;
				}

				current->left_child = NULL;
				current->right_child = NULL;
				break;
			}
			
			case ')': current = current->parent; break;
			case '!': next_is_negated = !next_is_negated; break;

			case '&':
			case '|':
			{
				current->operand = c;
				break;
			}

			default:
			{
				if (current->left_var == 0)
				{
					current->left_var = c;
					current->left_negated = next_is_negated;
				}
				else if (current->right_var == 0 && (i + 1 == len || logex[i + 1] == ')'))
				{
					current->right_var = c;
					current->right_negated = next_is_negated;
				}
				else
				{
					current->right_child = calloc(1, sizeof(LogexTree));
					current->right_child->parent = current;
					current->right_negated = next_is_negated;
					current = current->right_child;
					current->left_child = NULL;
					current->right_child = NULL;

					current->left_var = c;
					current->left_negated = next_is_negated;
				}

				break;
			}
		}

		if(c != '!')
			next_is_negated = 0;
	}

	return root;
}

void evaluateAllOccupanciesDNF(LogexTree* root, Occupancy* occupancy, size_t num_variables, const char* var_names, size_t i)
{
	if (i == num_variables)
	{
		if (evaluateLogexTree(root, occupancy))
		{
			printf("(");
			for (size_t j = 0; j < num_variables - 1; j++)
			{
				if (occupancy->values[var_names[j] - 'A'] == 0)
					printf("!");

				printf("%c & ", var_names[j]);
			}

			if (occupancy->values[var_names[num_variables - 1] - 'A'] == 0)
				printf("!");

			printf("%c)", var_names[num_variables - 1]);

			size_t count = 0;
			for (size_t j = 0; j < num_variables; j++)
			{
				if (occupancy->values[var_names[j] - 'A'] == 1)
					count++;
			}

			if (count < num_variables)
				printf(" | ");
			else
				printf("\n");
		}

		return;
	}

	occupancy->values[var_names[i] - 'A'] = 0;
	evaluateAllOccupanciesDNF(root, occupancy, num_variables, var_names, i + 1);

	occupancy->values[var_names[i] - 'A'] = 1;
	evaluateAllOccupanciesDNF(root, occupancy, num_variables, var_names, i + 1);
}

void evaluateAllOccupanciesCNF(LogexTree* root, Occupancy* occupancy, size_t num_variables, const char* var_names, size_t i)
{
	if (i == num_variables)
	{
		if (!evaluateLogexTree(root, occupancy))
		{
			printf("(");
			for (size_t j = 0; j < num_variables - 1; j++)
			{
				if (occupancy->values[var_names[j] - 'A'] == 0)
					printf("!");

				printf("%c | ", var_names[j]);
			}

			if (occupancy->values[var_names[num_variables - 1] - 'A'] == 0)
				printf("!");

			printf("%c)", var_names[num_variables - 1]);

			size_t count = 0;
			for (size_t j = 0; j < num_variables; j++)
			{
				if (occupancy->values[var_names[j] - 'A'] == 1)
					count++;
			}

			if (count < num_variables)
				printf(" & ");
			else
				printf("\n");
		}

		return;
	}

	occupancy->values[var_names[i] - 'A'] = 0;
	evaluateAllOccupanciesCNF(root, occupancy, num_variables, var_names, i + 1);

	occupancy->values[var_names[i] - 'A'] = 1;
	evaluateAllOccupanciesCNF(root, occupancy, num_variables, var_names, i + 1);
}

void logexToDNF(const char* logex)
{
	LogexTree* root = logexToLogexTree(logex);
	
	printf("[Logex]: Input was: ");
	printLogexTree(root);

	size_t num_variables = 0;
	Occupancy visited;
	char var_names[26];
	for (size_t i = 0; i < 26; i++)
		visited.values[i] = 0;

	size_t len = strlen(logex);
	for (size_t i = 0; i < len; i++)
	{
		if (isalpha(logex[i]) && !visited.values[logex[i] - 'A'])
		{
			var_names[num_variables++] = logex[i];
			visited.values[logex[i] - 'A'] = 1;
		}
	}

	Occupancy occupancy;
	for (size_t i = 0; i < 26; i++)
		occupancy.values[i] = 0;

	printf("[Logex]: Input converted to DNF: ");
	evaluateAllOccupanciesDNF(root, &occupancy, num_variables, var_names, 0);
}

void logexToCNF(const char* logex)
{
	LogexTree* root = logexToLogexTree(logex);
	
	printf("[Logex]: Input was: ");
	printLogexTree(root);

	size_t num_variables = 0;
	Occupancy visited;
	char var_names[26];
	for (size_t i = 0; i < 26; i++)
		visited.values[i] = 0;

	size_t len = strlen(logex);
	for (size_t i = 0; i < len; i++)
	{
		if (isalpha(logex[i]) && !visited.values[logex[i] - 'A'])
		{
			var_names[num_variables++] = logex[i];
			visited.values[logex[i] - 'A'] = 1;
		}
	}

	Occupancy occupancy;
	for (size_t i = 0; i < 26; i++)
		occupancy.values[i] = 0;

	printf("[Logex]: Input converted to CNF: ");
	evaluateAllOccupanciesCNF(root, &occupancy, num_variables, var_names, 0);
}

//TODO:
// Multiple brackets seem to not work properly, negations as well
// Make the input !((A | B | C) & (!A | C)) work.
//Solution in DNF: (A & B & !C) | (A & !B & !C) | (!A & !B & !C)
//Solution in CNF: (!A | !B | !C) & (!A | B| !C) & (A | !B | !C) & (A | !B | C) & (A| B | !C)

int main(int argc, char** argv)
{
	clapRegisterFlag("help", 'h', CLAP_FLAG_OPT_ARG, NULL);
	clapRegisterFlag("DNF", CLAP_FLAG_NO_SHORT, CLAP_FLAG_REQ_ARG, NULL);
	clapRegisterFlag("CNF", CLAP_FLAG_NO_SHORT, CLAP_FLAG_REQ_ARG, NULL);

	while (clapParse(argc, argv))
	{
		if (clapParsedFlag("help", 'h'))
		{
			const char* arg = clapGetArg();

			if(arg == NULL)
				printf("[Logex]: Logex supports the following flags:\n[Logex]: --DNF\n[Logex]: --CNF\n[Logex]: A logex is a logical expression.\n");
			else
			{
				if (strcmp(arg, "DNF") == 0)
					printf("[Logex]: --DNF: In boolean logic, a disjunctive normal form (DNF) is a canonical normal form of a logical formula consisting of a disjunction of conjunctions; it can also be described as an OR of ANDs, a sum of products, or (in philosophical logic) a cluster concept.\n[Logex]: Usage: --DNF <logex>");

				if (strcmp(arg, "CNF") == 0)
					printf("[Logex]: --CNF: In Boolean logic, a formula is in conjunctive normal form (CNF) or clausal normal form if it is a conjunction of one or more clauses, where a clause is a disjunction of literals; otherwise put, it is a product of sums or an AND of ORs. As a canonical normal form, it is useful in automated theorem proving and circuit theory.\n[Logex]: Usage: --CNF <logex>");
			}
		}

		if (clapParsedFlag("DNF", CLAP_FLAG_NO_SHORT))
			logexToDNF(clapGetArg());

		if (clapParsedFlag("CNF", CLAP_FLAG_NO_SHORT))
			logexToCNF(clapGetArg());
	}

	return 0;
}