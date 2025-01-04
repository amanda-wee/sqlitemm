Usage Examples
--------------
Building on the game results example in the main README, here is an example of saving a container of game results using a transaction:
```cpp
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
        statement.reset();
    }
    transaction.commit();
}
```
If there is an exception before the commit, or if the commit is omitted entirely, then the insertions will be rolled back.

Suppose instead that some scores have not been recorded, and so you want them to be set to NULL in the `score` database column until they can be updated for the given player:
```cpp
void save_results(const std::vector<GameResult>& game_results)
{
    auto connection = sqlitemm::Connection(DATABASE_FILENAME);
    auto transaction = connection.begin_transaction();
    auto statement = connection.prepare("INSERT INTO result (name, score) VALUES (:name, :score);");
    for (const auto& game_result : game_results)
    {
        statement[":name"] = game_result.get_name();
        if (game_result.is_recorded())
        {
            statement[":score"] = game_result.get_score();
        }
        else
        {
            statement[":score"] = nullptr;
        }
        statement.execute();
        statement.reset();
    }
    transaction.commit();
}
```
