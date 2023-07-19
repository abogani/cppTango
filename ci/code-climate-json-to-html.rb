#! /usr/bin/env ruby
# frozen_string_literal: true

require 'erb'
require 'json'
require 'cc/analyzer/filesystem'
require 'cc/analyzer/source_buffer'
require 'cc/analyzer/formatters/formatter'
require 'cc/analyzer/formatters/html_formatter'

# Limit the number of the issues to not crash the browser.
ISSUES_LIMIT = 3000

class JSONToHTMLEngine # rubocop:disable Style/Documentation
  def name
    'json-to-html'
  end
end

source_directory = ARGV[0] || Dir.pwd
filesystem = CC::Analyzer::Filesystem.new(source_directory)
formatter = CC::Analyzer::Formatters::HTMLFormatter.new(filesystem)

data = JSON.parse!($stdin.read)

formatter.started
formatter.engine_running(JSONToHTMLEngine.new) do
  data.take(ISSUES_LIMIT).each do |entry|
    formatter.write(JSON.generate(entry))
  end
end
formatter.finished
formatter.close
