import asyncio
import json
import logging
import os
from asyncio import Semaphore
from contextlib import asynccontextmanager
from datetime import datetime

from dotenv import load_dotenv
from web3 import AsyncWeb3
from web3.providers import AsyncHTTPProvider

from eth_analyzer.net_client import NetClient

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S",
)


@asynccontextmanager
async def multi_acquire(semaphore, count):
    for _ in range(count):
        await semaphore.acquire()
    try:
        yield
    finally:
        for _ in range(count):
            semaphore.release()


async def get_wallet_info(
    w3: AsyncWeb3, address: str, rpc_semaphore: Semaphore
) -> dict:
    address = w3.to_checksum_address(address)
    async with multi_acquire(rpc_semaphore, 2):
        nonce, balance = await asyncio.gather(
            w3.eth.get_transaction_count(address), w3.eth.get_balance(address)
        )
    return {
        "address": address,
        "nonce": nonce,
        "balance": float(w3.from_wei(balance, "ether")),
    }


async def get_transactions(
    w3: AsyncWeb3, address: str, max_count: int, alchemy_semaphore: Semaphore
) -> list:
    address = w3.to_checksum_address(address)
    hex_count = f"0x{max_count:x}"
    categories = ["internal", "external", "erc20"]

    params_from = [
        {
            "fromAddress": address,
            "maxCount": hex_count,
            "category": categories,
            "order": "desc",
            "withMetadata": True,
        }
    ]
    params_to = [
        {
            "toAddress": address,
            "maxCount": hex_count,
            "category": categories,
            "order": "desc",
            "withMetadata": True,
        }
    ]
    async with multi_acquire(alchemy_semaphore, 2):
        res_from, res_to = await asyncio.gather(
            w3.provider.make_request("alchemy_getAssetTransfers", params_from),
            w3.provider.make_request("alchemy_getAssetTransfers", params_to),
        )

    transfers = []

    if isinstance(res_from, dict) and "result" in res_from:
        result_data = res_from["result"]
        if result_data and "transfers" in result_data:
            transfers.extend(result_data["transfers"])

    if isinstance(res_to, dict) and "result" in res_to:
        result_data = res_to["result"]
        if result_data and "transfers" in result_data:
            transfers.extend(result_data["transfers"])

    transfers.sort(key=lambda t: int(t["blockNum"], 16), reverse=True)

    return transfers[:max_count]


async def produce_neighbours(w3, neighbours, rpc_semaphore):
    BANCH_SIZE = 2
    neighbours_data = []
    for i in range(0, len(neighbours), BANCH_SIZE):
        if i + BANCH_SIZE <= len(neighbours):
            neighbour_tasks = [
                get_wallet_info(w3, addr, rpc_semaphore)
                for addr in neighbours[i : i + BANCH_SIZE]
            ]
        else:
            neighbour_tasks = [
                get_wallet_info(w3, addr, rpc_semaphore)
                for addr in neighbours[i : len(neighbours)]
            ]
        tmp_data = await asyncio.gather(*neighbour_tasks)
        neighbours_data.extend(tmp_data)
        await asyncio.sleep(0.1)
    return neighbours_data


async def get_transactions_with_neighbours(
    w3: AsyncWeb3,
    address: str,
    max_count: int,
    rpc_semaphore: Semaphore,
    alchemy_semaphore: Semaphore,
    task_semaphore: Semaphore,
) -> dict:
    async with task_semaphore:
        address = w3.to_checksum_address(address)
        target_node, transactions = await asyncio.gather(
            get_wallet_info(w3, address, rpc_semaphore),
            get_transactions(w3, address, max_count, alchemy_semaphore),
        )

        neighbours = set()
        edges = []

        for tx in transactions:
            if tx and tx.get("to") and tx.get("from"):
                sender = w3.to_checksum_address(tx.get("from"))
                receiver = w3.to_checksum_address(tx.get("to"))

                if sender == address:
                    neighbours.add(receiver)
                elif receiver == address:
                    neighbours.add(sender)

                raw_timestamp = tx.get("metadata", {}).get("blockTimestamp")
                unix_time = 0

                if raw_timestamp:
                    dt = datetime.fromisoformat(raw_timestamp.replace("Z", "+00:00"))
                    unix_time = int(dt.timestamp())
                edge = {
                    "from": sender,
                    "to": receiver,
                    "value": tx.get("value"),
                    "timestamp": unix_time,
                    "id": tx.get("id"),
                }
                edges.append(edge)

        if neighbours:
            neighbours_data = await produce_neighbours(
                w3, list(neighbours), rpc_semaphore
            )

        else:
            neighbours_data = []

        res = {
            "target_node": target_node,
            "edges": edges,
            "neighbours_data": neighbours_data,
        }
        return res


async def producer(
    w3,
    address,
    max_height,
    max_deep,
    queue,
    rpc_semaphore,
    alchemy_semaphore,
    task_semaphore,
):
    processed = set()
    neighbours = [address]

    for i in range(max_deep):
        unique_neighbours = [n for n in neighbours if n not in processed]
        if not unique_neighbours:
            break

        for n in unique_neighbours:
            processed.add(n)

        tasks = []
        for n in unique_neighbours:
            task = asyncio.create_task(
                get_transactions_with_neighbours(
                    w3, n, max_height, rpc_semaphore, alchemy_semaphore, task_semaphore
                )
            )
            tasks.append(task)

        res = await asyncio.gather(*tasks, return_exceptions=True)

        neighbours = []
        for n1 in res:
            if isinstance(n1, Exception):
                logging.error(f"Error processing wallet: {n1}")
                continue
            await queue.put(n1)
            for n2 in n1.get("neighbours_data", []):
                addr = n2.get("address")
                if addr and addr not in processed:
                    neighbours.append(addr)


def item_log(item):
    target_log = item.get("target_node", {})
    nonce_log = target_log.get("nonce") if target_log else None
    address_log = target_log.get("address", "")[:8] if target_log else None
    edges_log = []
    for i in item.get("edges", []):
        edges_log.append((str(i.get("to")))[2:8])

    logging.info(
        "Consumed new wallet with: nonce = %s address = %s  neighbours = %s",
        nonce_log,
        address_log,
        edges_log,
    )


async def consumer(queue):
    counter = 0
    edge_counter = 0
    with NetClient("127.0.0.1", 7008) as client:
        try:
            while True:
                try:
                    item = await asyncio.wait_for(queue.get(), timeout=2.0)
                    client.send_json(item)
                    item_log(item)
                    counter += 1
                    edge_counter += len(item.get("edges", []))
                    queue.task_done()
                except asyncio.TimeoutError:
                    if queue.empty():
                        continue
        except asyncio.CancelledError:
            while not queue.empty():
                try:
                    item = queue.get_nowait()
                    client.send_json(item)
                    item_log(item)
                    counter += 1
                    edge_counter += len(item.get("edges", []))
                    queue.task_done()
                except asyncio.QueueEmpty:
                    break

            end = {"end": "yes"}
            client.send_json(end)
            logging.info(
                "Finish: total handled wallets counter = %d edges counter = %d",
                counter,
                edge_counter,
            )
            res_data = client.get_json()
            for elem in res_data:
                if elem.get("res") == True:
                    print(
                        "[INFO] result of check: ",
                        elem.get("level"),
                        " ",
                        elem.get("res_string"),
                    )
                else:
                    print("[INFO] result of check: ", elem.get("res_string"))


async def global_func(address, width, height):
    load_dotenv()
    RLC_URL = os.getenv("ALCHEMY_HTTP_URL")
    if not RLC_URL:
        raise ValueError("Error: ALCHEMY_HTTP_URL variable not found in .env file!")

    w3 = AsyncWeb3(AsyncHTTPProvider(RLC_URL))

    if not (await w3.is_connected()):
        raise ValueError(
            f"Error: bad ALCHEMY_HTTP_URL, connection corrupted: {RLC_URL}"
        )

    logging.info("[PARSER] connection was successful")

    rpc_semaphore = Semaphore(50)
    alchemy_semaphore = Semaphore(5)
    task_semaphore = Semaphore(2)

    queue = asyncio.Queue()

    producer_task = asyncio.create_task(
        producer(
            w3,
            address,
            width,
            height,
            queue,
            rpc_semaphore,
            alchemy_semaphore,
            task_semaphore,
        )
    )
    consumer_task = asyncio.create_task(consumer(queue))

    try:
        await producer_task
        await queue.join()
    finally:
        consumer_task.cancel()
        try:
            await consumer_task
        except asyncio.CancelledError:
            pass


def main():
    print("Enter the start wallet address (or press Enter to use Vitalik's address): ")
    start_wallet = input()
    if len(start_wallet) == 0:
        start_wallet = "0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045"
    print("Enter the width of the dependency graph: ")
    width = int(input())
    print("Enter the height of the dependency graph: ")
    height = int(input())
    try:
        asyncio.run(global_func(start_wallet, width, height))
    except Exception as e:
        logging.error(str(e))


if __name__ == "__main__":
    main()
