#!/bin/bash

# Path to the root directory containing all benchmark folders
BENCHMARKS_ROOT_DIR="misc/tests/benchmarks"

# Output file
OUTPUT_FILE="summary2.txt"

# Remove the existing output file if it exists
rm -f "$OUTPUT_FILE"

# Create the output file if it doesn't exist
touch "$OUTPUT_FILE"

# Append header to the output file if it's empty
if [ ! -s "$OUTPUT_FILE" ]; then
    echo -e "Problem_Name\tPlan_Length\tPlan_Cost\tTotal_Time\tExpanded\tReopened\tEvaluated\tGenerated" >> "$OUTPUT_FILE"
fi

# Initialize total runtime
TOTAL_RUNTIME=0

# Iterate over all subdirectories in the benchmarks root directory
for BENCHMARK_DIR in "$BENCHMARKS_ROOT_DIR"/*; do
    # Check if it's a directory
    if [ -d "$BENCHMARK_DIR" ]; then
        # Print folder name
        echo "" >> "$OUTPUT_FILE"
        echo "$(basename "$BENCHMARK_DIR")" >> "$OUTPUT_FILE"

        # Domain file name (assumed to be located in the current benchmark folder)
        DOMAIN_FILE="$BENCHMARK_DIR/domain.pddl"

        # Count the total number of problems
        NUM_PROBLEMS=$(find "$BENCHMARK_DIR" -maxdepth 1 -type f -name "*.pddl" | grep -v "$(basename "$DOMAIN_FILE")" | wc -l)
        CURRENT_PROBLEM=1

        # Start time for folder
        FOLDER_START_TIME=$(date +%s.%N)

        # Iterate over all PDDL problem files in the current benchmark directory
        for PROBLEM_FILE in "$BENCHMARK_DIR"/*.pddl; do
            # Skip the domain file
            if [ "$(basename "$PROBLEM_FILE")" == "$(basename "$DOMAIN_FILE")" ]; then
                continue
            fi

            # Run Fast Downward on the current problem
            PROBLEM_NAME=$(basename "$PROBLEM_FILE")
            echo "Processing problem $CURRENT_PROBLEM of $NUM_PROBLEMS: $PROBLEM_NAME"

            # Redirect all output to a temporary file
            TEMP_OUTPUT=$(mktemp)
            python3 fast-downward.py \
                --overall-time-limit 5m \
                "$DOMAIN_FILE" "$PROBLEM_FILE" \
                --evaluator "hff=ff()" \
                --search "eager(open=type_based([hff], random_seed=-1))" \
                > "$TEMP_OUTPUT" 2>&1

            # Extract relevant information
            PLAN_LENGTH=$(awk '/Plan length:/ {print $(NF-1)}' "$TEMP_OUTPUT")
            PLAN_COST=$(awk '/Plan cost:/ {print $NF}' "$TEMP_OUTPUT")
            TOTAL_TIME=$(awk '/Total time:/ {print $NF}' "$TEMP_OUTPUT")
            EXPANDED=$(awk '/Expanded/ {print $(NF-1)}' "$TEMP_OUTPUT")
            REOPENED=$(awk '/Reopened/ {print $(NF-1)}' "$TEMP_OUTPUT")
            EVALUATED=$(awk '/Evaluated/ {print $(NF-1)}' "$TEMP_OUTPUT")
            GENERATED=$(awk '/Generated/ && !/rules/ {print $(NF-1)}' "$TEMP_OUTPUT")

            # Append information to the output file
            echo -e "$(basename "$PROBLEM_FILE")\t$PLAN_LENGTH\t$PLAN_COST\t$TOTAL_TIME\t$EXPANDED\t$REOPENED\t$EVALUATED\t$GENERATED" >> "$OUTPUT_FILE"

            # Clean up temporary file
            rm "$TEMP_OUTPUT"

            # Increment the current problem count
            ((CURRENT_PROBLEM++))
        done

        # End time for folder
        FOLDER_END_TIME=$(date +%s)

        # Calculate runtime for folder
        FOLDER_RUNTIME=$(echo "$FOLDER_END_TIME - $FOLDER_START_TIME" | bc -l)

        # Add folder runtime to total runtime
        TOTAL_RUNTIME=$(echo "$TOTAL_RUNTIME + $FOLDER_RUNTIME" | bc)
        
        # Print folder runtime
        echo "Folder runtime: $FOLDER_RUNTIME seconds" >> "$OUTPUT_FILE"
    fi
done

# Print total runtime at the end of the output file
echo "Total runtime: $TOTAL_RUNTIME seconds" >> "$OUTPUT_FILE"
