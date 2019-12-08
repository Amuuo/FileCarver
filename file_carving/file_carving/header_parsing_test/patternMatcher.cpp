
#include "patternMatcher.h"


  /*

int main()
{
  printf("\nPattern Matcher");

  int n = 4;
  PatternMatcher PM{};
  vector<string> queries(n);

  queries[0] = "adab";
  queries[1] = "dab";
  queries[2] = "ab";
  queries[3] = "b";

  string s = "abracadabra";

  PM.create_goto(queries);
  PM.find_matches(s);

  printf("\nPM.positions.size(): %d", PM.positions.size());
  printf("\nprinting positions:\n\n");

  printf("\n\nString to search: %s\n\n", s.c_str());

  for (int i = 0; i < PM.positions.size(); i++)
  {
    printf("%-5s: ", queries[i].c_str());
    for (list<int>::iterator itr = PM.positions[i].begin(); itr != PM.positions[i].end(); ++itr)
      printf("%d ", *itr - (queries[i].size() - 1));

    printf("\n");
  }


  printf("\ngo_to:");

  for (auto go : PM.go_to) {
    for (auto state : go.second) {
      printf("\n%c", state.first);
    }
  }

  printf("\noutput: ");

  for (auto output : PM.output) {
    for (auto state : output.second) {
      printf("\n%d", state);
    }
  }
  return 0;
}
  */






void PatternMatcher::find_matches(uint8_t* tmp, disk_pos blocksize)
{
  printf("\n\nFunction: find_matches()");
  int state = 0, t;
  for (int i = 0; i < blocksize; i++) {
    while (go_to[state].count(tmp[i]) == 0) {
      printf("\ngo_to[state].count(S[i]): %i", (int)go_to[state].count(tmp[i]));
      state = failure[state];
    }
    state = go_to[state][tmp[i]];
    printf("\nstate: %d", state);

    if (output.count(state) > 0)
    {
      t = output[state].size();
      vector<int> tmp = output[state];

      for (int j = 0; j < t; j++)
      {
        printf("\npushing back %d", i);
        positions[tmp[j]].push_back(i);
      }
    }

  }

}






void PatternMatcher::create_goto(const vector<FileHeader>& patterns)
{
  positions.resize(patterns.size());
  int new_state = 0;

  for (int i = 0; i < patterns.size(); i++)
  {
    int state = 0, j;

    for (j = 0; j < patterns.size() && go_to[state].count(patterns[i].header_byte_array[j]) >= 1; j++)
    {
      state = go_to[state][patterns[i].header_byte_array[j]];
    }

    for (int k = j; k < patterns.size(); k++)
    {
      go_to[state][patterns[i].header_byte_array[k]] = ++new_state;
      state = new_state;
    }

    if (output.find(state) == output.end())
      output[state] = vector<int>(1, i);
    else
    {
      output[state].push_back(i);
    }
  }
  failure.resize(new_state + 1, 0);

  for (int i = 0; i <= 0xFF; i++)
    if (go_to[0].count(i) == 0)
      go_to[0][i] = 0;

  create_failure();

}





void PatternMatcher::create_failure() {
  queue<int> q;
  int current, state;
  for (map<int, uint8_t>::iterator itr = go_to[0].begin(); itr != go_to[0].end(); ++itr) {
    if (itr->second != 0)
    {
      q.push(itr->second);
      failure[itr->second] = 0;
    }
  }

  while (!q.empty())
  {
    current = q.front();
    printf("\npopping %d", current);
    q.pop();
    state = failure[current];

    for (map<int, uint8_t>::iterator itr = go_to[current].begin(); itr != go_to[current].end(); itr++)
    {
      printf("\nitr: %x", (int)itr->first);
      if (go_to[current].count(itr->first) != 0)
      {
        printf("\npushing %d", itr->second);
        q.push(itr->second);
        state = failure[current];
        while (go_to[state].count(itr->first) == 0)
        {
          state = failure[state];
        }
        failure[itr->second] = go_to[state][itr->first];

        output[itr->second].insert(output[itr->second].end(),
          output[failure[itr->second]].begin(),
          output[failure[itr->second]].end());
      }
    }
  }
}