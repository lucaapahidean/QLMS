-- Drop and recreate database
DROP DATABASE IF EXISTS qlms;
CREATE DATABASE qlms;

-- Connect to qlms database
\c qlms;

-- Create tables
CREATE TABLE users (
    user_id SERIAL PRIMARY KEY,
    username VARCHAR(100) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    role VARCHAR(20) CHECK (role IN ('admin', 'instructor', 'student')) NOT NULL
);

CREATE TABLE classes (
    class_id SERIAL PRIMARY KEY,
    class_name VARCHAR(255) UNIQUE NOT NULL
);

CREATE TABLE courses (
    course_id SERIAL PRIMARY KEY,
    course_name VARCHAR(255) NOT NULL,
    class_id INTEGER NOT NULL REFERENCES classes(class_id) ON DELETE CASCADE
);

CREATE TABLE class_members (
    class_id INTEGER NOT NULL REFERENCES classes(class_id) ON DELETE CASCADE,
    user_id INTEGER NOT NULL REFERENCES users(user_id) ON DELETE CASCADE,
    PRIMARY KEY (class_id, user_id)
);

CREATE TABLE course_materials (
    material_id SERIAL PRIMARY KEY,
    title VARCHAR(255) NOT NULL,
    type VARCHAR(20) CHECK (type IN ('lesson', 'quiz')) NOT NULL,
    course_id INTEGER REFERENCES courses(course_id) ON DELETE SET NULL,
    creator_id INTEGER REFERENCES users(user_id) ON DELETE SET NULL
);

CREATE TABLE text_lessons (
    lesson_id INTEGER PRIMARY KEY REFERENCES course_materials(material_id) ON DELETE CASCADE,
    content TEXT NOT NULL
);

CREATE TABLE quizzes (
    quiz_id INTEGER PRIMARY KEY REFERENCES course_materials(material_id) ON DELETE CASCADE,
    max_attempts INTEGER NOT NULL DEFAULT 1,
    feedback_type VARCHAR(30) CHECK (feedback_type IN (
        'detailed_with_answers',     -- Shows wrong/right + correct answers
        'detailed_without_answers',  -- Shows wrong/right but not correct answers
        'score_only'                 -- Shows only the score
    )) NOT NULL DEFAULT 'detailed_with_answers'
);

CREATE TABLE questions (
    question_id SERIAL PRIMARY KEY,
    quiz_id INTEGER NOT NULL REFERENCES quizzes(quiz_id) ON DELETE CASCADE,
    prompt TEXT NOT NULL,
    question_type VARCHAR(20) CHECK (question_type IN ('checkbox', 'radio', 'open_answer')) NOT NULL
);

CREATE TABLE question_options (
    option_id SERIAL PRIMARY KEY,
    question_id INTEGER NOT NULL REFERENCES questions(question_id) ON DELETE CASCADE,
    option_text TEXT NOT NULL,
    is_correct BOOLEAN NOT NULL DEFAULT FALSE
);

CREATE TABLE quiz_attempts (
    attempt_id SERIAL PRIMARY KEY,
    quiz_id INTEGER NOT NULL REFERENCES quizzes(quiz_id) ON DELETE CASCADE,
    student_id INTEGER NOT NULL REFERENCES users(user_id) ON DELETE CASCADE,
    attempt_number INTEGER NOT NULL,
    status VARCHAR(30) CHECK (status IN ('completed', 'pending_manual_grading')) NOT NULL,
    auto_score FLOAT,           -- Score from auto-graded questions
    manual_score FLOAT,         -- Score from manually graded questions
    final_score FLOAT,          -- Total score
    total_auto_points INTEGER,  -- Total possible points from auto-graded questions
    total_manual_points INTEGER,-- Total possible points from manual questions
    submitted_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    graded_at TIMESTAMP,
    UNIQUE(quiz_id, student_id, attempt_number)
);

CREATE TABLE answers (
    answer_id SERIAL PRIMARY KEY,
    attempt_id INTEGER NOT NULL REFERENCES quiz_attempts(attempt_id) ON DELETE CASCADE,
    question_id INTEGER NOT NULL REFERENCES questions(question_id) ON DELETE CASCADE,
    student_response TEXT,
    is_correct BOOLEAN,         -- NULL for open_answer, TRUE/FALSE for auto-graded
    points_earned FLOAT,        -- Points earned for this answer
    max_points FLOAT            -- Maximum possible points for this question
);

-- Create indexes for better performance
CREATE INDEX idx_users_role ON users(role);
CREATE INDEX idx_course_materials_type ON course_materials(type);
CREATE INDEX idx_quiz_attempts_status ON quiz_attempts(status);
CREATE INDEX idx_quiz_attempts_student ON quiz_attempts(student_id);
CREATE INDEX idx_answers_attempt ON answers(attempt_id);
CREATE INDEX idx_course_materials_course_id ON course_materials(course_id);
CREATE INDEX idx_class_members_user_id ON class_members(user_id);