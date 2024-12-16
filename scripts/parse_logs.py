import argparse
import os


def analyze_log_content(content):
    lines = content.splitlines()
    num_lines = len(lines)
    num_words = sum(len(line.split()) for line in lines)
    return {"lines": num_lines, "words": num_words}

def process_experiment_logs(base_dir):
    results = {}

    for experiment in os.listdir(base_dir):
        experiment_path = os.path.join(base_dir, experiment)
        if not os.path.isdir(experiment_path):
            print(f"{experiment_path} is not a folder")
            return

        results[experiment] = {}
        
        for repetition in os.listdir(experiment_path):
            repetition_path = os.path.join(experiment_path, repetition)
            if not os.path.isdir(repetition_path):
                print(f"{repetition_path} is not a subfolder")

            stats = {
                "latency": 0,
                "throughput": 0,
                "block_size": 0,
                "chunk_size": 0,
            }

            log_files = os.listdir(repetition_path)
            for log_file in log_files:
                log_file_path = os.path.join(repetition_path, log_file)

                if not os.path.isfile(log_file_path):
                    print(f"{log_file_path} is not a log file")

                with open(log_file_path, 'r') as file:
                    content = file.read()
                    analysis_result = analyze_log_content(content)
                    stats["latency"] += analysis_result["latency"]
                    stats["throughput"] += analysis_result["throughput"]
                    stats["block_size"] += analysis_result["block_size"]
                    stats["chunk_size"] += analysis_result["chunk_size"]
            
            stats["latency"] = round(stats["latency"] / len(log_files))
            stats["throughput"] = round(stats["throughput"] / len(log_files))
            stats["block_size"] = round(stats["block_size"] / len(log_files))
            stats["chunk_size"] = round(stats["chunk_size"] / len(log_files))
            results[experiment] = stats
    
    return results


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate log summary of experiments')
    parser.add_argument('--folder', type=str, default='./experiments')
    parser.add_argument('--runningTime', type=int, default=300)
    args = parser.parse_args()
