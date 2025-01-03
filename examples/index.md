Usage Examples
--------------
Building on the game results example in the main README, here is an example of saving a container of game results using a transaction:
```C
void save_results(const std::vector<GameResult>& game_results)
{
    auto connection = sqlitemm::Connection(DATABASE_FILENAME);
    auto transaction = connection.begin_transaction();
    auto statement = connection.prepare("INSERT INTO result (name, score) VALUES (:name, :score);");
    for (const auto& game_result : game_results)
    {
        statement[":name"] = game_result.get_name();
        statement[":score"] = game_result.get_score();
        statement.execute();
    }
    transaction.commit();
}
```
If there is an exception before the commit, or if the commit is omitted entirely, then the insertions will be rolled back.
